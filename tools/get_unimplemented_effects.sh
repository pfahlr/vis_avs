#!/usr/bin/env bash
set -euo pipefail

export REPO_ROOT="$(git rev-parse --show-toplevel)"

python3 <<'PY'
import os
import pathlib
import re
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

original_root = repo / "docs/avs_original_source/vis_avs"


def read_text(path: pathlib.Path) -> str:
    return path.read_text(errors="ignore")


def normalize_tokens(label: str) -> tuple[str, ...]:
    label = label.lower()
    if "/" in label:
        label = label.split("/")[-1]
    tokens = re.findall(r"[a-z0-9]+", label)
    filtered = [
        t
        for t in tokens
        if t not in {"render", "trans", "misc", "effect", "mode", "winamp", "nullsoft", "builtin"}
    ]
    if not filtered:
        filtered = tokens
    return tuple(sorted(filtered))


class_to_name: dict[str, str] = {}
for header in header_paths:
    if not header.is_file():
        continue
    current_class = None
    for line in header.read_text().splitlines():
        class_match = re.match(r"\s*class\s+(\w+)", line)
        if class_match:
            current_class = class_match.group(1)
        name_match = re.search(r'return\s+"([^"]+)"', line)
        if current_class and name_match:
            class_to_name[current_class] = name_match.group(1)
        if current_class and line.strip().startswith('};'):
            current_class = None

registered: dict[str, str] = {}
if registry_path.is_file():
    for line in registry_path.read_text().splitlines():
        m = re.search(r'REG\("([^"]+)",\s*([A-Za-z0-9_]+),', line)
        if m:
            effect_id, cls = m.group(1), m.group(2)
            registered[cls] = effect_id

keywords = ("stub", "placeholder", "todo", "unimplemented")

source_text: dict[str, str] = {str(path): read_text(path) for path in source_paths if path.is_file()}

class_to_stub: dict[str, bool] = {}


def remove_comments(code: str) -> str:
    code = re.sub(r'//.*', '', code)
    code = re.sub(r'/\*.*?\*/', '', code, flags=re.S)
    return code


def preceding_comment(text: str, idx: int) -> str:
    i = idx
    lines: list[str] = []
    while i > 0:
        j = text.rfind('\n', 0, i)
        if j == -1:
            segment = text[:i]
            if segment.strip().startswith('//'):
                lines.append(segment.strip())
            break
        segment = text[j + 1:i].strip()
        if not segment:
            break
        if segment.startswith('//'):
            lines.append(segment)
            i = j
            continue
        if segment.endswith('*/'):
            k = text.rfind('/*', 0, j)
            if k == -1:
                break
            block = text[k:j].splitlines()
            lines.extend(s.strip() for s in block if s.strip())
            i = k
            continue
        break
    return '\n'.join(reversed(lines))


for path_str, text in source_text.items():
    for match in re.finditer(r'void\s+([A-Za-z0-9_]+)::process\s*\([^)]*\)\s*\{', text):
        cls = match.group(1)
        start = match.end()
        depth = 1
        i = start
        while i < len(text) and depth > 0:
            if text[i] == '{':
                depth += 1
            elif text[i] == '}':
                depth -= 1
            i += 1
        body = text[start:i - 1]
        comment_text = preceding_comment(text, match.start())
        body_lower = body.lower()
        comment_lower = comment_text.lower()

        is_stub = False
        if any(k in body_lower for k in keywords) or any(k in comment_lower for k in keywords):
            is_stub = True
        else:
            simplified = remove_comments(body)
            simplified = simplified.replace('{', '').replace('}', '')
            simplified_no_ws = re.sub(r'\s+', '', simplified)
            trivial_patterns = [
                r'\(void\)ctx;',
                r'\(void\)dst;',
                r'return;',
                r'MovementEffect\(\)\.process\(ctx,dst\);',
            ]
            tmp = simplified_no_ws
            for pat in trivial_patterns:
                tmp = re.sub(pat, '', tmp)
            if not tmp:
                is_stub = True

        class_to_stub[cls] = is_stub


def parse_original_effects() -> list[dict[str, str]]:
    if not original_root.is_dir():
        return []
    rlib_path = original_root / "rlib.cpp"
    if not rlib_path.is_file():
        return []

    text = read_text(rlib_path)

    def extract_block(func_name: str) -> str | None:
        try:
            start = text.index(func_name)
        except ValueError:
            return None
        brace = text.find('{', start)
        if brace == -1:
            return None
        depth = 1
        i = brace + 1
        while i < len(text) and depth > 0:
            if text[i] == '{':
                depth += 1
            elif text[i] == '}':
                depth -= 1
            i += 1
        return text[brace + 1 : i - 1]

    fx_block = extract_block("void C_RLibrary::initfx")
    ape_block = extract_block("void C_RLibrary::initbuiltinape")

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

    effect_files = {p.stem: p for p in original_root.glob('r_*.cpp')}
    fx_info: list[dict[str, str]] = []

    for func in fx_functions:
        file_path = None
        mod_name = None
        for path in effect_files.values():
            text_cpp = read_text(path)
            if re.search(rf'\b{re.escape(func)}\b', text_cpp):
                file_path = path
                # find the last MOD_NAME defined before the function occurrence
                func_pos = text_cpp.find(func)
                search_region = text_cpp[:func_pos]
                matches = list(re.finditer(r'#define\s+MOD_NAME\s+"([^"]+)"', search_region))
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

implemented_tokens = set()
stub_tokens = set()

for cls, name in class_to_name.items():
    tokens = normalize_tokens(name)
    if not tokens:
        continue
    if class_to_stub.get(cls):
        stub_tokens.add(tokens)
    elif cls in registered:
        implemented_tokens.add(tokens)

stubbed = []
for cls, effect_id in registered.items():
    if class_to_stub.get(cls):
        name = class_to_name.get(cls, cls)
        stubbed.append((name, effect_id, cls))
stubbed.sort()

missing_original = []
if original_effects:
    # Group originals by normalized token signature
    grouped: dict[tuple[str, ...], list[dict[str, str]]] = defaultdict(list)
    for entry in original_effects:
        key = normalize_tokens(entry["name"])
        if not key:
            key = (entry["function"].lower(),)
        grouped[key].append(entry)

    for key, entries in grouped.items():
        if key in implemented_tokens:
            continue
        if key in stub_tokens:
            # Implemented but still stubbed; leave reporting to stubbed list
            continue
        for entry in entries:
            missing_original.append(entry)

    missing_original.sort(key=lambda e: (e["name"].lower(), e["function"]))

if stubbed:
    print("Stubbed effects registered in avs-core:")
    for name, effect_id, cls in stubbed:
        print(f"  {effect_id}: {name} [{cls}]")

if missing_original:
    if stubbed:
        print()
    print("Effects from original Winamp AVS sources not yet implemented:")
    for entry in missing_original:
        print(
            f"  {entry['name']} (source: {entry['function']} in {entry['path']})"
        )

if not stubbed and not missing_original:
    print("All registered effects are implemented and in parity with original sources.")
PY
