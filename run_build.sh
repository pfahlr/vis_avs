#!/bin/bash
set -euo pipefail

print_help() {
  echo "usage $0 <options>"
  echo "options:"
  echo "  --help: print this help screen"
  echo "  --clear-build-dir: remove existing build directory"
  echo "  --dryrun: don't actually run, just print what it would have done"
}

clear_build=false
dryrun=false

while [[ $# -gt 0 ]]; do
  case "$1" in
    --help)
      print_help
      exit 0
      ;;
    --clear-build-dir)
      clear_build=true
      shift 1
      ;;
    --dryrun)
      dryrun=true
      shift 1
      ;;
    *)
      # Ignore unknown positional arguments for now
      shift 1
      ;;
  esac
done

if [[ "${clear_build}" == true ]]; then
  echo "clearing build directory"
  if [[ "${dryrun}" == true ]]; then
    echo "[dryrun] rm -rf build"
  else
    rm -rf build
  fi
fi

echo "make build directory if not exists, cd into build"
if [[ "${dryrun}" == true ]]; then
  echo "[dryrun] mkdir -p build"
  echo "[dryrun] cd build"
else
  mkdir -p build
  cd build
fi

echo "run cmake on parent '..'"
if [[ "${dryrun}" == true ]]; then
  echo "[dryrun] cmake .."
else
  cmake ..
fi

echo "run cmake --build ."
if [[ "${dryrun}" == true ]]; then
  echo "[dryrun] cmake --build ."
else
  cmake --build .
fi

echo "run ctest"
if [[ "${dryrun}" == true ]]; then
  echo "[dryrun] ctest --output-on-failure"
else
  ctest --output-on-failure
fi
