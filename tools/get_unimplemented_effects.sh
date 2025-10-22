#!/usr/bin/env bash
set -euo pipefail

export REPO_ROOT="$(git rev-parse --show-toplevel)"

python3 <<'PY'
import json
import os
import pathlib
import re
import subprocess
from collections import defaultdict

repo = pathlib.Path(os.environ["REPO_ROOT"])

registry_path = repo / "libs/avs-compat/src/registry.cpp"
source_paths = [
    repo / "libs/avs-compat/src/effects_render.cpp",
    repo / "libs/avs-compat/src/effects_trans.cpp",
    repo / "libs/avs-compat/src/effects_misc.cpp",
]
header_paths = [
    repo / "libs/avs-compat/include/avs/compat/effects_render.hpp",
    repo / "libs/avs-compat/include/avs/compat/effects_trans.hpp",
    repo / "libs/avs-compat/include/avs/compat/effects_misc.hpp",
]

dev_plan_path = repo / "codex/specs/visual_effects/components/core_visual_effects.yaml"
original_root = repo / "docs/avs_original_source/vis_avs"
preset_source_path = repo / "libs/avs-compat/src/preset.cpp"


def read_text(path: pathlib.Path) -> str:
    return path.read_text(errors="ignore") if path.is_file() else ""


def normalize_tokens(label: str) -> tuple[str, ...]:
    label = label.lower()
    if "/" in label:
        label = label.split("/")[-1]
    tokens = re.findall(r"[a-z0-9]+", label)
    filtered = [
        t
        for t in tokens
        if t
        not in {
            "render",
            "trans",
            "misc",
            "effect",
            "mode",
            "winamp",
            "nullsoft",
            "builtin",
        }
    ]
    if not filtered:
        filtered = tokens
    return tuple(sorted(filtered))


def extract_block(text: str, start_index: int) -> tuple[str, int]:
    depth = 1
    i = start_index
    start = i
    while i < len(text) and depth > 0:
        ch = text[i]
        if ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
        i += 1
    return text[start : i - 1], i


def remove_comments(code: str) -> str:
    code = re.sub(r"//.*", "", code)
    code = re.sub(r"/\*.*?\*/", "", code, flags=re.S)
    return code


def preceding_comment(text: str, idx: int) -> str:
    i = idx
    lines: list[str] = []
    while i > 0:
        j = text.rfind("\n", 0, i)
        if j == -1:
            segment = text[:i]
            if segment.strip().startswith("//"):
                lines.append(segment.strip())
            break
        segment = text[j + 1 : i].strip()
        if not segment:
            break
        if segment.startswith("//"):
            lines.append(segment)
            i = j
            continue
        if segment.endswith("*/"):
            k = text.rfind("/*", 0, j)
            if k == -1:
                break
            block = text[k:j].splitlines()
            lines.extend(s.strip() for s in block if s.strip())
            i = k
            continue
        break
    return "\n".join(reversed(lines))


class_metadata: dict[str, dict[str, str | None]] = {}
for header in header_paths:
    text = read_text(header)
    if not text:
        continue
    lines = text.splitlines()
    i = 0
    while i < len(lines):
        line = lines[i]
        class_match = re.match(r"\s*class\s+(\w+)", line)
        if not class_match:
            i += 1
            continue
        cls = class_match.group(1)
        metadata = class_metadata.setdefault(
            cls,
            {
                "header": str(header.relative_to(repo)),
                "display_name": None,
                "group_hint": None,
            },
        )
        depth = line.count("{") - line.count("}")
        j = i + 1
        while j < len(lines):
            current = lines[j]
            depth += current.count("{") - current.count("}")
            if "name() const override" in current:
                name_match = re.search(r'"([^"]+)"', current)
                if name_match:
                    metadata["display_name"] = name_match.group(1)
            if "EffectGroup" in current and "return" in current:
                group_match = re.search(r"EffectGroup::([A-Za-z]+)", current)
                if group_match:
                    metadata["group_hint"] = group_match.group(1).lower()
            if depth <= 0:
                break
            j += 1
        i = j + 1


class_to_stub: dict[str, bool] = {}
class_to_source: dict[str, str] = {}
parameters_by_class: dict[str, list[dict[str, str]]] = {}
keywords = ("stub", "placeholder", "todo", "unimplemented")

for path in source_paths:
    text = read_text(path)
    if not text:
        continue
    for match in re.finditer(r"void\s+([A-Za-z0-9_]+)::process\s*\([^)]*\)\s*\{", text):
        cls = match.group(1)
        body, end_idx = extract_block(text, match.end())
        comment_text = preceding_comment(text, match.start())
        body_lower = body.lower()
        comment_lower = comment_text.lower()
        is_stub = False
        if any(k in body_lower for k in keywords) or any(k in comment_lower for k in keywords):
            is_stub = True
        else:
            simplified = remove_comments(body)
            simplified = simplified.replace("{", "").replace("}", "")
            simplified_no_ws = re.sub(r"\s+", "", simplified)
            trivial_patterns = [
                r"\(void\)ctx;",
                r"\(void\)dst;",
                r"return;",
                r"MovementEffect\(\)\.process\(ctx,dst\);",
            ]
            tmp = simplified_no_ws
            for pat in trivial_patterns:
                tmp = re.sub(pat, "", tmp)
            if not tmp:
                is_stub = True
        class_to_stub[cls] = is_stub
        class_to_source[cls] = str(path.relative_to(repo))
    for match in re.finditer(r"std::vector<Param>\s+([A-Za-z0-9_]+)::parameters\(\) const\s*\{", text):
        cls = match.group(1)
        body, _ = extract_block(text, match.end())
        params: list[dict[str, str]] = []
        for pm in re.finditer(r'Param\s*\{\s*"([^"]+)"\s*,\s*ParamKind::([A-Za-z0-9_]+)', body):
            params.append({"name": pm.group(1), "kind": pm.group(2)})
        parameters_by_class[cls] = params


registry_entries: list[tuple[str, str, str]] = []
if registry_path.is_file():
    text = read_text(registry_path)
    for match in re.finditer(r'REG\("([^"]+)",\s*([A-Za-z0-9_]+),\s*EffectGroup::([A-Za-z]+)\)', text):
        effect_id, cls, group = match.groups()
        registry_entries.append((effect_id, cls, group.lower()))


def parse_original_effects() -> list[dict[str, str]]:
    if not original_root.is_dir():
        return []
    rlib_path = original_root / "rlib.cpp"
    if not rlib_path.is_file():
        return []
    text = read_text(rlib_path)

    def extract_block_from_keyword(keyword: str) -> str | None:
        try:
            start = text.index(keyword)
        except ValueError:
            return None
        brace = text.find("{", start)
        if brace == -1:
            return None
        block, _ = extract_block(text, brace + 1)
        return block

    fx_block = extract_block_from_keyword("void C_RLibrary::initfx")
    ape_block = extract_block_from_keyword("void C_RLibrary::initbuiltinape")

    fx_functions: list[str] = []
    if fx_block:
        fx_functions.extend(
            m.group(1)
            for m in re.finditer(r'DECLARE_EFFECT2?\((R_[A-Za-z0-9_]+)\);', fx_block)
        )

    ape_entries: list[tuple[str, str]] = []
    if ape_block:
        for m in re.finditer(r'ADD2\(\s*(R_[A-Za-z0-9_]+)\s*,\s*"([^"]+)"\s*\)', ape_block):
            ape_entries.append((m.group(1), m.group(2)))

    effect_files = {p.stem: p for p in original_root.glob("r_*.cpp")}
    fx_info: list[dict[str, str]] = []

    for func in fx_functions:
        file_path = None
        mod_name = None
        for path in effect_files.values():
            text_cpp = read_text(path)
            if re.search(rf"\b{re.escape(func)}\b", text_cpp):
                file_path = path
                func_pos = text_cpp.find(func)
                search_region = text_cpp[:func_pos]
                matches = list(
                    re.finditer(r'#define\s+MOD_NAME\s+"([^"]+)"', search_region)
                )
                if matches:
                    mod_name = matches[-1].group(1)
                else:
                    mod_name = func
                break
        if file_path is None:
            continue
        fx_info.append(
            {
                "function": func,
                "name": mod_name or func,
                "path": str(file_path.relative_to(repo)),
            }
        )

    for func, name in ape_entries:
        fx_info.append(
            {
                "function": func,
                "name": name,
                "path": str((original_root / "rlib.cpp").relative_to(repo)),
            }
        )

    return fx_info


original_effects = parse_original_effects()
original_by_tokens: defaultdict[tuple[str, ...], list[dict[str, str]]] = defaultdict(list)
for entry in original_effects:
    key = normalize_tokens(entry["name"])
    if not key:
        key = (entry["function"].lower(),)
    original_by_tokens[key].append(entry)


def load_dev_plan() -> dict[tuple[str, ...], dict[str, str]]:
    if not dev_plan_path.is_file():
        return {}
    entries: list[dict[str, str]] = []
    current: dict[str, str] | None = None
    for raw_line in read_text(dev_plan_path).splitlines():
        stripped = raw_line.strip()
        if not stripped or stripped.startswith("#"):
            continue
        if not raw_line.startswith(" "):
            if current:
                entries.append(current)
                current = None
            continue
        if stripped.startswith("- name:"):
            if current:
                entries.append(current)
            name = stripped.split(":", 1)[1].strip().strip('"')
            current = {"name": name}
        elif current:
            if stripped.startswith("category:"):
                current["category"] = stripped.split(":", 1)[1].strip()
            elif stripped.startswith("description:"):
                current["description"] = stripped.split(":", 1)[1].strip().strip('"')
    if current:
        entries.append(current)
    plan_map: dict[tuple[str, ...], dict[str, str]] = {}
    for entry in entries:
        name = entry.get("name")
        if not name:
            continue
        key = normalize_tokens(name)
        if key and key not in plan_map:
            plan_map[key] = entry
    return plan_map


def gather_references(terms: list[str]) -> list[dict[str, str | int]]:
    results: list[dict[str, str | int]] = []
    seen: set[tuple[str, int, str]] = set()
    search_roots = [repo / "tests", repo / "codex", repo / "apps"]
    for term in terms:
        if not term:
            continue
        for root in search_roots:
            if not root.exists():
                continue
            cmd = [
                "rg",
                "--with-filename",
                "--line-number",
                "--no-heading",
                "--ignore-case",
                "--fixed-strings",
                term,
                str(root),
            ]
            try:
                proc = subprocess.run(
                    cmd, capture_output=True, text=True, check=False
                )
            except FileNotFoundError:
                return []
            if proc.returncode not in (0, 1):
                continue
            for line in proc.stdout.splitlines():
                parts = line.split(":", 2)
                if len(parts) < 3:
                    continue
                file_path, line_no, snippet = parts
                try:
                    line_int = int(line_no)
                except ValueError:
                    continue
                rel_path = os.path.relpath(file_path, repo)
                key = (rel_path, line_int, snippet.strip())
                if key in seen:
                    continue
                seen.add(key)
                results.append(
                    {
                        "path": rel_path.replace("\\", "/"),
                        "line": line_int,
                        "match": snippet.strip(),
                        "term": term,
                    }
                )
    return results


plan_by_tokens = load_dev_plan()


def parse_effect_name_array(text: str, array_name: str) -> list[tuple[int, str, str]]:
    pattern = re.compile(
        r"%s\s*=\s*\{(?P<body>.*?)\};" % re.escape(array_name),
        re.S,
    )
    match = pattern.search(text)
    if not match:
        return []
    body = match.group("body")
    entries: list[tuple[int, str, str]] = []
    entry_pattern = re.compile(
        r'"([^"]*)"\s*,?\s*//\s*([A-Za-z0-9_]+)',
    )
    index = 0
    for m in entry_pattern.finditer(body):
        label = m.group(1)
        symbol = m.group(2)
        entries.append((index, label, symbol))
        index += 1
    return entries


def parse_preset_reference_map() -> dict[tuple[str, ...], list[dict[str, object]]]:
    text = read_text(preset_source_path)
    if not text:
        return {}
    registered_entries = parse_effect_name_array(text, "kRegisteredEffectNames")
    legacy_entries = parse_effect_name_array(text, "kLegacyEffectNames")
    by_symbol: dict[str, dict[str, object]] = {}
    for idx, label, symbol in registered_entries:
        entry = by_symbol.setdefault(
            symbol,
            {
                "index": idx,
                "registered_label": None,
                "legacy_label": None,
            },
        )
        entry["index"] = idx
        if label:
            entry["registered_label"] = label
    for idx, label, symbol in legacy_entries:
        entry = by_symbol.setdefault(
            symbol,
            {
                "index": idx,
                "registered_label": None,
                "legacy_label": None,
            },
        )
        entry.setdefault("index", idx)
        if label:
            entry["legacy_label"] = label
    token_map: dict[tuple[str, ...], list[dict[str, object]]] = defaultdict(list)
    for symbol, info in by_symbol.items():
        index = int(info.get("index", 0))
        for kind in ("registered_label", "legacy_label"):
            label = info.get(kind)
            if not label:
                continue
            tokens = normalize_tokens(str(label))
            if not tokens:
                continue
            token_map[tokens].append(
                {
                    "legacy_symbol": symbol,
                    "effect_index": index,
                    "label": label,
                    "kind": "registered_name" if kind == "registered_label" else "legacy_name",
                }
            )
    return token_map


preset_reference_by_tokens = parse_preset_reference_map()


def build_preset_reference_support(tokens: tuple[str, ...]) -> dict[str, object]:
    matches = preset_reference_by_tokens.get(tokens, [])
    if not matches:
        return {
            "available": False,
            "legacy_symbol": None,
            "effect_index": None,
            "methods": [],
        }
    # Prefer registered names when present for the primary metadata.
    def sort_key(item: dict[str, object]) -> tuple[int, str]:
        return (0 if item.get("kind") == "registered_name" else 1, str(item.get("label", "")))

    ordered = sorted(matches, key=sort_key)
    primary = ordered[0]
    methods = []
    seen: set[tuple[str, str]] = set()
    for item in ordered:
        label = str(item.get("label", ""))
        kind = str(item.get("kind", ""))
        key = (kind, label)
        if key in seen:
            continue
        seen.add(key)
        methods.append({"kind": kind, "label": label})
    return {
        "available": True,
        "legacy_symbol": primary.get("legacy_symbol"),
        "effect_index": primary.get("effect_index"),
        "methods": methods,
    }


effects: list[dict[str, object]] = []
covered_tokens: set[tuple[str, ...]] = set()

for effect_id, cls, group in registry_entries:
    metadata = class_metadata.get(cls, {})
    display_name = metadata.get("display_name") if metadata else None
    tokens = normalize_tokens(display_name or effect_id)
    if not tokens:
        tokens = (effect_id.lower(),)
    covered_tokens.add(tokens)
    original_matches = original_by_tokens.get(tokens, [])
    plan_entry = plan_by_tokens.get(tokens)
    implementation_files: dict[str, str] = {}
    header_path = metadata.get("header") if metadata else None
    if header_path:
        implementation_files["header"] = header_path
    source_path = class_to_source.get(cls)
    if source_path:
        implementation_files["source"] = source_path
    parameters = parameters_by_class.get(cls, [])
    references = gather_references(
        [
            effect_id,
            display_name or "",
            f"{group} / {display_name}" if display_name else "",
        ]
    )
    status = "stub" if class_to_stub.get(cls, False) else "implemented"
    origin = "winamp_original" if original_matches else "2025_rebuild_only"
    effect_entry = {
        "machine_readable_effect_name": f"{group}/{effect_id}",
        "status": status,
        "implementation_complete": status == "implemented",
        "group": group,
        "registry_id": effect_id,
        "class_name": cls,
        "display_name": display_name,
        "origin": origin,
        "tokens": list(tokens),
        "in_dev_plan": bool(plan_entry),
        "dev_plan": (
            {
                **plan_entry,
                "source": str(dev_plan_path.relative_to(repo)),
            }
            if plan_entry
            else None
        ),
        "implementation_files": implementation_files or None,
        "parameters": parameters,
        "original_sources": original_matches,
        "preset_references_count": len(references),
        "preset_reference_support": build_preset_reference_support(tokens),
    }
    if status == "stub":
        effect_entry["notes"] = "Process() currently marked as stub"
    effects.append(effect_entry)

for tokens, entries in original_by_tokens.items():
    if tokens in covered_tokens:
        continue
    plan_entry = plan_by_tokens.get(tokens)
    category = plan_entry.get("category") if plan_entry else None
    for entry in entries:
        machine_suffix = "_".join(tokens) if tokens else entry["function"].lower()
        references = gather_references([entry["name"], entry["function"]])
        effects.append(
            {
                "machine_readable_effect_name": f"legacy/{machine_suffix}",
                "status": "missing",
                "implementation_complete": False,
                "group": category.lower() if category else None,
                "registry_id": None,
                "class_name": None,
                "display_name": entry["name"],
                "origin": "winamp_original",
                "tokens": list(tokens),
                "in_dev_plan": bool(plan_entry),
                "dev_plan": (
                    {
                        **plan_entry,
                        "source": str(dev_plan_path.relative_to(repo)),
                    }
                    if plan_entry
                    else None
                ),
                "implementation_files": None,
                "parameters": [],
                "original_sources": [entry],
                "preset_references_count": len(references),
                "preset_reference_support": build_preset_reference_support(tokens),
                "notes": "No compat layer stub or registry entry",
            }
        )

for tokens, plan_entry in plan_by_tokens.items():
    if tokens in covered_tokens:
        continue
    if tokens in original_by_tokens:
        continue
    machine_suffix = "_".join(tokens)
    references = gather_references([plan_entry.get("name", "")])
    effects.append(
        {
            "machine_readable_effect_name": f"plan_only/{machine_suffix}",
            "status": "planned_only",
            "implementation_complete": False,
            "group": plan_entry.get("category"),
            "registry_id": None,
            "class_name": None,
            "display_name": plan_entry.get("name"),
            "origin": "2025_rebuild_only",
            "tokens": list(tokens),
            "in_dev_plan": True,
            "dev_plan": {
                **plan_entry,
                "source": str(dev_plan_path.relative_to(repo)),
            },
            "implementation_files": None,
            "parameters": [],
            "original_sources": [],
            "preset_references_count": len(references),
            "preset_reference_support": build_preset_reference_support(tokens),
            "notes": "Tracked in development plan only",
        }
    )


status_priority = {
    "missing": 0,
    "planned_only": 1,
    "stub": 2,
    "implemented": 3,
}
effects.sort(
    key=lambda e: (
        status_priority.get(e.get("status"), len(status_priority)),
        e.get("machine_readable_effect_name", ""),
    )
)
print(json.dumps(effects, indent=2, sort_keys=True))
PY
