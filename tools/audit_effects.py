#!/usr/bin/env python3
import re, sys, json, pathlib, yaml, subprocess

ROOT = pathlib.Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "docs" / "effects_manifest.yaml"

CPP_GLOBS = [
    "libs/avs-effects-legacy/src/**/*.cpp",
    "libs/avs-effects-legacy/include/**/*.h",
    "libs/avs-effects-legacy/include/**/*.hpp",
    "libs/avs-compat/src/**/*.cpp",
]

TOKEN_RE = re.compile(r'AVS_EFFECT_TOKEN\s*\(\s*"([^"]+)"\s*\)')  # or your token macro
CLASS_RE = re.compile(r'\bclass\s+([A-Za-z_]\w*)')
EFFECT_NAME_HEUR = re.compile(r'effect_([a-z0-9_]+)\.(?:h|hpp|cpp)$')

def git_ls_files(pattern):
    try:
        out = subprocess.check_output(["git", "ls-files", pattern], cwd=ROOT, text=True, stderr=subprocess.DEVNULL)
        return [ROOT / p for p in out.strip().splitlines() if p]
    except Exception:
        return list(ROOT.glob(pattern))

def scan_files():
    files = []
    for g in CPP_GLOBS:
        files += git_ls_files(g)
    return files

def extract_tokens(text):
    return TOKEN_RE.findall(text)

def main():
    manifest = yaml.safe_load(MANIFEST.read_text())
    canon = {e["token"]: e for e in manifest["effects"]}

    seen = {}  # token -> list[file]
    entries = []

    for f in scan_files():
        t = f.read_text(errors="ignore")
        tokens = extract_tokens(t)
        classes = CLASS_RE.findall(t)
        m = EFFECT_NAME_HEUR.search(f.name)
        heuristic = m.group(1) if m else None

        for tok in tokens or ["<no-token>"]:
            seen.setdefault(tok, []).append(str(f))
        entries.append({
            "path": str(f),
            "tokens": tokens,
            "classes": classes,
            "heuristic_name": heuristic,
        })

    # Compute status
    missing = [c for c in canon if c not in seen]
    extras  = [tok for tok in seen if tok not in canon and tok != "<no-token>"]
    duplicates = {tok: paths for tok, paths in seen.items() if len(paths) > 1 and tok != "<no-token>"}

    report = {
        "totals": {
            "manifest_effects": len(canon),
            "files_scanned": len(entries),
        },
        "missing_tokens": missing,
        "extra_tokens": extras,
        "duplicate_tokens": duplicates,
        "monolith_hits": [e for e in entries if "avs-compat/src/effects_" in e["path"]],
        "entries": entries[:200],  # trim
    }
    print(json.dumps(report, indent=2))

if __name__ == "__main__":
    main()
