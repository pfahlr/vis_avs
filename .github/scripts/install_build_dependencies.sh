#!/usr/bin/env bash
set -euo pipefail

DISTRO="${DISTRO:-}"

if [[ -z "${DISTRO}" ]]; then
  echo "DISTRO environment variable must be provided" >&2
  exit 1
fi

case "${DISTRO}" in
  ubuntu)
    export DEBIAN_FRONTEND=noninteractive
    apt-get update
    apt-get install -y --no-install-recommends \
      build-essential \
      ccache \
      clang \
      cmake \
      git \
      ninja-build \
      pkg-config \
      python3 \
      libsdl2-dev \
      libgl1-mesa-dev \
      libglu1-mesa-dev \
      libasound2-dev \
      libjack-dev \
      libportaudio2 \
      portaudio19-dev \
      libsamplerate0-dev \
      libfftw3-dev
    ;;
  fedora)
    dnf -y makecache
    dnf -y install \
      ccache \
      clang \
      cmake \
      gcc \
      gcc-c++ \
      git \
      ninja-build \
      pkgconf-pkg-config \
      python3 \
      SDL2-devel \
      mesa-libGL-devel \
      mesa-libGLU-devel \
      alsa-lib-devel \
      jack-audio-connection-kit-devel \
      portaudio-devel \
      libsamplerate-devel \
      fftw-devel
    ;;
  *)
    echo "Unsupported distro: ${DISTRO}" >&2
    exit 1
    ;;
esac
