# AVS Cross-Platform Port

This project aims to reimplement Winamp's Advanced Visualization Studio (AVS) as a standalone,
portable engine and tooling.

## Prerequisites

Ensure the following packages are installed:

```bash
sudo apt-get install cmake g++ clang-format git pkg-config \
  libsdl2-dev mesa-common-dev libglu1-mesa-dev \
  portaudio19-dev libportaudio2 libgtest-dev libsamplerate0-dev \
  libjack-dev libasound2-dev
```

## Build Instructions

```bash
git clone <repo-url>
cd avs-cross-platform  # repository root
mkdir build && cd build
cmake ..
cmake --build .
ctest
```

Run the stub player after building:

```bash
./apps/avs-player/avs-player [--sample-rate 48000] [--channels 2]
```

The runtime can request specific capture parameters when talking to PortAudio.
Use `--sample-rate` and `--channels` to ask the device for a particular format;
the player will fall back to the device defaults if the request is not
available and resample to the engine's 48 kHz internal representation.

To drive rendering from a WAV file, run the player in headless mode. Supplying
`--wav` without `--headless` will terminate with an error.

```bash
./apps/avs-player/avs-player --headless --wav tests/data/test.wav \
  --preset tests/data/simple.avs --frames 120 --out output_dir
```

More documentation will be added as the project evolves.
