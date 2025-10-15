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

### Building the `avs-player` binary

The interactive/headless CLI lives under `apps/avs-player` and is produced
whenever the SDL/OpenGL platform layer is part of the build. The new default
sets `-DAVS_BUILD_PLATFORM=ON`, so the standard configure and build commands
above generate `build/apps/avs-player/avs-player` without any extra flags.

If you previously disabled the platform layer (for example,
`cmake .. -DAVS_BUILD_PLATFORM=OFF`), rerun configuration with
`-DAVS_BUILD_PLATFORM=ON` to bring the binary back. Developers with a system
PortAudio installation may optionally add `-DENABLE_PORTAUDIO=ON`; otherwise
the default `OFF` setting will fetch a suitable copy automatically.

Run the stub player after building:

```bash
./apps/avs-player/avs-player [--sample-rate 48000|default] [--channels 2|default]
```

The runtime can request specific capture parameters when talking to PortAudio.
Use `--sample-rate` and `--channels` to ask the device for a particular format;
pass `default` to either flag to explicitly request the device defaults. The
player falls back to the hardware defaults when the request is not available
and resamples to the engine's 48 kHz internal representation. Library consumers
can achieve the same behaviour by adjusting `AudioInputConfig::requestedSampleRate`,
`AudioInputConfig::requestedChannels`, and the new `useDeviceDefaultSampleRate`
and `useDeviceDefaultChannels` toggles.

Enumerate capture devices (and their numeric identifiers) via:

```bash
./apps/avs-player/avs-player --list-input-devices
```

Pick a device either by index or by matching part of its name:

```bash
./apps/avs-player/avs-player --input-device 2
./apps/avs-player/avs-player --input-device "USB"
```

If the chosen endpoint cannot capture audio, the player reports a descriptive
error instead of silently falling back to the default input.

Embedding applications can mirror this behaviour through
`AudioInputConfig::requestedDeviceIdentifier`. Set the optional string to a
PortAudio device index or to a substring of the desired device name before
constructing `avs::AudioInput`.

To drive rendering from a WAV file, run the player in headless mode. Supplying
`--wav` without `--headless` will terminate with an error.

```bash
./apps/avs-player/avs-player --headless --wav tests/data/test.wav \
  --preset tests/data/simple.avs --frames 120 --out output_dir
```

More documentation will be added as the project evolves.
