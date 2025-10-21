#!/bin/bash

set -euo pipefail

function print_help {
  echo "usage $0 <options>"
  echo "options:"
  echo "  --help: print this help screen"
  echo "  --platform <value>: which linux distro to setup (ubuntu|fedora)"
  echo "  --dryrun: don't actually run, just print what it would have done"
}

platform="ubuntu"
dryrun=false

while [[ $# -gt 0 ]]; do
  case "$1" in
    --help)
      print_help
      exit 0
      ;;
    --platform)
      if [[ $# -lt 2 ]]; then
        echo "--platform requires an argument" >&2
        exit 1
      fi
      platform="$2"
      shift 2
      ;;
    --dryrun)
      dryrun=true
      shift 1
      ;;
    *)
      echo "Unknown option: $1" >&2
      exit 1
      ;;
  esac
done

case "$platform" in
  ubuntu|fedora)
    ;;
  *)
    echo "$platform not supported" >&2
    exit 1
    ;;
esac

if command -v sudo >/dev/null 2>&1 && [[ "$EUID" -ne 0 ]]; then
  sudo_cmd=(sudo)
else
  sudo_cmd=()
fi

run_cmd() {
  if [[ "$dryrun" == true ]]; then
    printf '[dryrun]'
    for arg in "$@"; do
      printf ' %q' "$arg"
    done
    printf '\n'
  else
    "$@"
  fi
}

case "$platform" in
  ubuntu)
    packages=(
      build-essential
      clang
      cmake
      git
      ninja-build
      pkg-config
      python3
      libsdl2-dev
      libgl1-mesa-dev
      libglu1-mesa-dev
      libasound2-dev
      libjack-dev
      libportaudio2
      portaudio19-dev
      libsamplerate0-dev
      libfftw3-dev
    )
    echo "Installing Ubuntu build dependencies (mirrors CI)"
    run_cmd "${sudo_cmd[@]}" apt-get update
    run_cmd "${sudo_cmd[@]}" apt-get install -y --no-install-recommends "${packages[@]}"
    ;;
  fedora)
    packages=(
      clang
      cmake
      gcc
      gcc-c++
      git
      ninja-build
      pkgconf-pkg-config
      python3
      SDL2-devel
      mesa-libGL-devel
      mesa-libGLU-devel
      alsa-lib-devel
      jack-audio-connection-kit-devel
      portaudio-devel
      libsamplerate-devel
      fftw-devel
    )
    echo "Installing Fedora build dependencies (mirrors CI)"
    run_cmd "${sudo_cmd[@]}" dnf -y makecache
    run_cmd "${sudo_cmd[@]}" dnf -y install "${packages[@]}"
    ;;
esac

