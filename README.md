# AVS Cross-Platform Port

This repository hosts a cross-platform reimplementation of Winamp's Advanced Visualization Studio (AVS).
The legacy Nullsoft source is preserved under `docs/avs_original_source` for reference.

See [docs/README.md](docs/README.md) for build and contribution instructions.


## build dependencies

`cmake g++ clang-format git pkg-config` 

### Fedora

`SDL2-devel mesa-libGL-devel mesa-libGLU-devel mesa-libGLU-devel portaudio-devel portaudio gtest-devel jack-audio-connection-kit-devel  alsa-lib-devel`

### Debian 

`libsdl2-dev mesa-common-dev libglu1-mesa-dev portaudio19-dev libportaudio2 libgtest-dev libjack-dev libasound2-dev`
