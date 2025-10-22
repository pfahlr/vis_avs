# AVS Cross-Platform Port

[![CI](https://github.com/pfahlr/vis_avs/actions/workflows/ci.yml/badge.svg?branch=development-branch)](https://github.com/pfahlr/vis_avs/actions/workflows/ci.yml)

This project aims to reimplement Winamp's Advanced Visualization Studio (AVS) as a standalone,
portable engine and tooling.

## Prerequisites

Ensure the following packages are installed. The helper script mirrors the CI
environment and pulls in the PortAudio development headers automatically.

```bash
# Ubuntu container
./run_setup_dev_environment.sh --platform ubuntu

# Fedora container
./run_setup_dev_environment.sh --platform fedora

# Manual apt invocation (Ubuntu) if you prefer running the commands yourself
sudo apt-get install cmake g++ clang-format git pkg-config \
  libsdl2-dev mesa-common-dev libglu1-mesa-dev \
  portaudio19-dev libportaudio2 libgtest-dev libsamplerate0-dev \
  libjack-dev libasound2-dev
```

> **Tip:** CMake now invokes a lightweight bootstrapper that runs the helper
> script automatically when the required development packages are missing.
> Pass `AVS_SKIP_AUTO_DEPS=1` in the environment to disable the automatic
> installation if you prefer to manage dependencies manually.

## Build Instructions

```bash
git clone <repo-url>
cd avs-cross-platform  # repository root
mkdir build && cd build
cmake ..
cmake --build .
ctest
```

## Build Options

Configure the project with `-D<option>=<ON|OFF>` to toggle these components:

| Option | Default | Description |
| --- | --- | --- |
| `AVS_BUILD_PLAYER` | `ON` | Build the Winamp-compatible `apps/avs-player` binary. |
| `AVS_BUILD_TOOLS` | `ON` | Compile utilities under `tools/`, including `gen_golden_md5`. |
| `AVS_ENABLE_SDL3` | `OFF` | Experimental SDL3 backend switch; defaults to SDL2 when disabled. |
| `AVS_INSTALL_RESOURCES` | `ON` | Install the asset payload under `share/vis_avs/resources/`. |

Installed artifacts follow the Unix-style layout: executables in `bin/`, libraries in `lib/vis_avs/`, headers in `include/vis_avs/`, and resources in `share/vis_avs/resources/`.

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

## Resource discovery

Runtime assets live under the repository's `resources/` directory. CMake copies
this tree next to the build outputs at `${CMAKE_BINARY_DIR}/resources` and, when
`AVS_INSTALL_RESOURCES` is enabled (the default), installs it under
`${CMAKE_INSTALL_PREFIX}/share/vis_avs/resources`.

At runtime the engine resolves resources with the following priority:

1. The directory referenced by the `AVS_RESOURCE_DIR` environment variable.
2. The build-tree mirror (`${CMAKE_BINARY_DIR}/resources`).
3. The install-tree payload (`${CMAKE_INSTALL_PREFIX}/share/vis_avs/resources`).

Override `AVS_RESOURCE_DIR` to point the player or tests at an alternative
payload, for example when packaging assets separately from the binaries.

More documentation will be added as the project evolves.

## Effects Core

The modern effects core lives under `libs/avs/core` and exposes a small,
deterministic pipeline. Effects are registered in an
`avs::core::EffectRegistry` and assembled into an `avs::core::Pipeline`. The
registry ships with a couple of utility effects (`clear`, `zoom`) from
`avs::effects::registerCoreEffects`.

```cpp
#include "avs/core/EffectRegistry.hpp"
#include "avs/core/Pipeline.hpp"
#include "avs/core/RenderContext.hpp"
#include "avs/effects/RegisterEffects.hpp"

avs::core::EffectRegistry registry;
avs::effects::registerCoreEffects(registry);

avs::core::Pipeline pipeline(registry);
avs::core::ParamBlock clearParams;
clearParams.setInt("value", 16);
pipeline.add("clear", clearParams);

std::vector<std::uint8_t> pixels(640 * 480 * 4, 0);
avs::core::RenderContext ctx;
ctx.width = 640;
ctx.height = 480;
ctx.framebuffer = {pixels.data(), pixels.size()};
ctx.frameIndex = 0;
ctx.deltaSeconds = 1.0 / 60.0;

pipeline.render(ctx);
```

Set the `AVS_SEED` environment variable (defaults to `0`) to guarantee
deterministic random number generation across runs when effects rely on the
provided `avs::core::DeterministicRng`.
