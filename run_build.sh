#!/bin/bash
function print_help {
  echo "usage $0 <options>"
  echo "options:"
  echo "  --help: print this help screen"
  echo "  --clear-build-dir: remove existing build directory"
  echo "  --dryrun: don't actually run, just print what it would have done"
}
#parse arguments
while [ $# -gt 0 ]; do
    case "$1" in
       	--help)
            print_help;
            exit 0
            ;;
        --clear-build-dir)
            clear_build="True"
            shift 1
            ;;
        --dryrun)
            dryrun="True"
            shift 1
            ;;
        *)
            # Handle positional arguments or unknown options
            text=$1
            shift 1
            ;;
    esac
done

if [[ ! -z "$clear_build" ]]; then
  echo "clearing build directory"
  rm -rf build
fi
echo "make build directory if not exists, cd into build"
mkdir -p build && cd build
echo "run cmake on parent '..'"
cmake ..
echo "run cmake --build ."
cmake --build .
echo "run ctest"
ctest

