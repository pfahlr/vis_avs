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
      cmake \
      git \
      libasound2-dev \
      libgl1-mesa-dev \
      libglu1-mesa-dev \
      libjack-dev \
      libportaudio2 \
      libsdl2-dev \
      libsamplerate0-dev \
      libfftw3-dev \
      python3
    ;;
  fedora)
    dnf -y makecache
    dnf -y install \
      cmake \
      SDL2-devel \
      alsa-lib-devel \
      fftw-devel \
      git \
      jack-audio-connection-kit-devel \
      libsamplerate-devel \
      mesa-libGL-devel \
      mesa-libGLU-devel \
      portaudio-devel \
      python3
    ;;
  *)
    echo "Unsupported distro: ${DISTRO}" >&2
    exit 1
    ;;
fi
