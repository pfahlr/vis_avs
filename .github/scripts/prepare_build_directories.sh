#!/usr/bin/env bash
set -euo pipefail

DISTRO="${DISTRO:-}"

if [[ -z "${DISTRO}" ]]; then
  echo "DISTRO environment variable must be provided" >&2
  exit 1
fi

mkdir -p build/_deps
mkdir -p .ccache
mkdir -p "$HOME/.cache/fetchcontent"

case "${DISTRO}" in
  ubuntu)
    mkdir -p /var/cache/apt
    ;;
  fedora)
    mkdir -p /var/cache/dnf
    ;;
  *)
    echo "Unsupported distro: ${DISTRO}" >&2
    exit 1
    ;;
fi
