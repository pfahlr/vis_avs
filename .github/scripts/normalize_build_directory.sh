#!/usr/bin/env bash
set -euo pipefail

DISTRO="${DISTRO:-}"

if [[ -z "${DISTRO}" ]]; then
  echo "DISTRO environment variable must be provided" >&2
  exit 1
fi

if [[ -d build ]]; then
  exit 0
fi

distro_dir="build-${DISTRO}"

maybe_extract_archive() {
  local archive="$1"
  local tmpdir

  if [[ ! -f "${archive}" ]]; then
    return 1
  fi

  tmpdir="$(mktemp -d)"
  trap 'rm -rf "${tmpdir}"' RETURN

  case "${archive}" in
    *.tar.gz|*.tgz)
      tar -xzf "${archive}" -C "${tmpdir}"
      ;;
    *.tar)
      tar -xf "${archive}" -C "${tmpdir}"
      ;;
    *.zip)
      python3 - "${archive}" "${tmpdir}" <<'PY'
import sys
import zipfile
from pathlib import Path

archive_path = Path(sys.argv[1])
destination = Path(sys.argv[2])

with zipfile.ZipFile(archive_path) as zf:
    zf.extractall(destination)
PY
      ;;
    *)
      echo "Unsupported archive format: ${archive}" >&2
      return 1
      ;;
  esac

  if [[ -d "${tmpdir}/build" ]]; then
    mv "${tmpdir}/build" build
  else
    local first_dir
    first_dir="$(find "${tmpdir}" -mindepth 1 -maxdepth 1 -type d | head -n 1)"
    if [[ -n "${first_dir}" ]]; then
      mv "${first_dir}" build
    else
      echo "Archive ${archive} did not contain a build directory" >&2
      return 1
    fi
  fi

  rm -rf "${tmpdir}"
  rm -f "${archive}"
  trap - RETURN
  return 0
}

if [[ -d "${distro_dir}/build" ]]; then
  mv "${distro_dir}/build" build
  rmdir "${distro_dir}" || true
  exit 0
fi

if [[ -d "${distro_dir}" ]]; then
  mv "${distro_dir}" build
  exit 0
fi

for archive in \
  "${distro_dir}.tar.gz" \
  "${distro_dir}.tgz" \
  "${distro_dir}.tar" \
  "${distro_dir}.zip"
do
  if maybe_extract_archive "${archive}"; then
    exit 0
  fi
done

echo "Unable to locate build directory after downloading artifact" >&2
echo "Contents of workspace:" >&2
ls -al >&2
exit 1
