#!/bin/bash

function print_help {
  echo "usage $0 <options>"
  echo "options:"
  echo "  --help: print this help screen"
  echo "  --platform <value>: which linux distro to setup"
  echo "  --dryrun: don't actually run, just print what it would have done"
}

PLATFORM="ubuntu"

#parse arguments
while [ $# -gt 0 ]; do
    case "$1" in
       	--help)
            print_help;
            exit 0
            ;;
        --platform)
            platform="$2"
            shift 2
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

# validate argument values
if [[ ! -z "$platform" ]]; then
  case "$platform" in 
    ubuntu)
      PLATFORM="ubuntu";
      ;;
    fedora)
      PLATFORM="fedora"
      ;;
    *)
      echo "$platform not supported"
      exit 0
  esac
fi

# run build enviornment setup
case $PLATFORM in 
    ubuntu)
       if [[ $dryrun == "True" ]]; then
         echo "dryrun ubuntu"
       else
         echo "installing ubuntu build system"
       sudo apt-get install cmake g++ clang-format git pkg-config \
       libsdl2-dev mesa-common-dev libglu1-mesa-dev \
       portaudio19-dev libportaudio2 libgtest-dev libsamplerate0-dev \
       libjack-dev libasound2-dev
       fi
       ;;
    fedora)
       echo "fedora not implemented yet"
       ;;
esac


