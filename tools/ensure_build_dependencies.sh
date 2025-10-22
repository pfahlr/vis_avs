#!/bin/bash
set -euo pipefail

if [[ $# -lt 1 ]]; then
  echo "Usage: $0 <build-dir>" >&2
  exit 1
fi

build_dir=$1
shift || true

if [[ -n "${AVS_SKIP_AUTO_DEPS:-}" ]]; then
  echo "AVS_SKIP_AUTO_DEPS is set; skipping automatic dependency installation." >&2
  exit 0
fi

sentinel="${build_dir}/.avs-deps-installed"
if [[ -f "${sentinel}" ]]; then
  exit 0
fi

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
setup_script="${script_dir}/../run_setup_dev_environment.sh"

if [[ ! -x "${setup_script}" ]]; then
  echo "Dependency setup script not found: ${setup_script}" >&2
  exit 1
fi

if [[ -r /etc/os-release ]]; then
  # shellcheck disable=SC1091
  source /etc/os-release
  case "${ID}" in
    ubuntu|debian)
      platform="ubuntu"
      ;;
    fedora)
      platform="fedora"
      ;;
    *)
      echo "Automatic dependency installation is not supported on distribution '${ID}'." >&2
      echo "Install dependencies manually or run run_setup_dev_environment.sh with the appropriate platform." >&2
      exit 1
      ;;
  esac
else
  echo "Unable to detect distribution (missing /etc/os-release)." >&2
  exit 1
fi

"${setup_script}" --platform "${platform}"

touch "${sentinel}"
