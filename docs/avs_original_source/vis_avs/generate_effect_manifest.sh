#!/usr/bin/env bash
set -euo pipefail

find . -type f -name 'r_*.cpp' -print0 |
while IFS= read -r -d '' f; do
  # Grab the first MOD_NAME line (if any)
  line=$(grep -m1 -E '^[[:space:]]*#define[[:space:]]+MOD_NAME' "$f" || true)
  if [[ -z "$line" ]]; then
    echo "file: $f had no MOD_NAME (skipped)" >&2
    continue
  fi

  # Token = everything after MOD_NAME, trim trailing comments/space
  token=$(printf '%s' "$line" \
    | sed -E 's@.*\bMOD_NAME[[:space:]]+@@; s@//.*$@@; s@/\*.*$@@; s/[[:space:]]+$//')

  # id: lowercased, collapse spaces around '/', then spaces->underscores; strip surrounding quotes first
  id=$(printf '%s' "$token" \
    | sed -E 's/^"(.*)"$/\1/' \
    | tr '[:upper:]' '[:lower:]' \
    | sed -E 's@[[:space:]]*/[[:space:]]*@/@g; s/[[:space:]]+/_/g')

  category=${id%%/*}
  base=$(basename "$f")

  printf '  - id: %s\n    original_file: %s\n    token: %s\n    category: %s\n    status: required\n' \
    "$id" "$base" "$token" "$category"
done
