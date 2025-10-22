#!/usr/bin/env bash
set -euo pipefail

DISTRO="${DISTRO:-}"
WORKSPACE="${GITHUB_WORKSPACE:-}"

if [[ -z "${WORKSPACE}" ]]; then
  echo "GITHUB_WORKSPACE environment variable must be provided" >&2
  exit 1
fi

if [[ "${DISTRO}" != "ubuntu" ]]; then
  exit 0
fi

GOOGLETEST_DIR="${WORKSPACE}/external/googletest"

test -d "${GOOGLETEST_DIR}/.git"
git -C "${GOOGLETEST_DIR}" rev-parse --short HEAD
