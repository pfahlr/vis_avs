# Advanced Visualization Studio 2K25 

This is a reimplementation of Winamp's Advanced Visualization Studio (AVS) created
to run natively on current hardware and graphics implementations. 

The idea behind the project is to provide an updated audio visualizer that takes
advantage of the technology improvements of the two decades between now and the
era of the original AVS, but to create a new visualization system while the
extensive library of presets created for AVS become less and less accessible 
didn't feel right. First of all like reinventing the wheel, and there also
seems to be this acceptance that digital artforms be lost to the obsolesence of the
technology they were created on. Imagine if we treated the music that they were 
created to enhance that way. Aside from that, I can't think of any reason why 
it would be better to create a project like this that could release with 2 decades
worth of supported media but instead release with just the handful I might create 
during development.

That being said. I do not intend for this project to stop at implementing effects needed 
to run the presets built for the original. Instead, once those have been implemented, 
I will begin to design new effects that either were not imagined 20 years ago or were
not possible on the technology of that time. There will be a standard interface
for implementing new effects. And it will be documented as soon as it is implemented. 
I welcome the collaboration of anyone who wishes to have some say in regards to what
that standard interface should be. It should be required that any such effects make
its source code available because 2 decades from now, it may be that it is time
to provide a 3rd generation of this software, and when that time comes, it will be
necessary to reimplement all of the effects. For a moment during this development
I thought it might not be possible to fully bring this software into the present day
because many of the presets I tested were dependent on 5 .ape (compiled windows dll)
effects. Fortunately, I was able to find the source code at 
[https://github.com/Const-me/vis_avs_dx/blob/master/avs_dx/DxVisuals/Effects/Trans/Convolution/Convolution.cpp]


The legacy Nullsoft source is preserved under `docs/avs_original_source` for reference.

See [docs/README.md](docs/README.md) for build and contribution instructions.


## Live audio capture in `avs-player`

`avs-player` uses PortAudio for real-time audio capture. The player selects an
input device automatically when no explicit identifier is provided. The default
policy prefers the first full-duplex device whose reported default sample rate
matches 48 kHz; if none is available, the first capture-capable endpoint is
used.

Inspect available devices with:

```bash
./apps/avs-player/avs-player --list-input-devices
```

Select a specific device by index or by its exact name:

```bash
./apps/avs-player/avs-player --input-device 0
./apps/avs-player/avs-player --input-device "Built-in Microphone"
```

You can still override the capture sample rate via `--sample-rate <hz>`; using
`--sample-rate default` (the default behaviour) requests the device's preferred
rate.

## Deterministic Rendering & Golden Hashes

The repository includes an offscreen renderer for repeatable image captures that
do not require a windowing system. Rendering is seeded via the `AVS_SEED`
environment variable (default `1234`) so that the effect pipeline receives a
stable sequence of random numbers. A dedicated regression test renders the
`tests/regression/data/tiny_preset_fragment.avs` preset for ten frames at
320×240, computes MD5 hashes of the raw RGBA buffer, and compares them against
`tests/regression/data/expected_md5_320x240_seed1234.json`.

To regenerate the snapshot after changing the renderer, run the CLI tool from
the build directory:

```bash
./tools/gen_golden_md5 --frames 10 --width 320 --height 240 --seed 1234 \
  --preset tests/regression/data/tiny_preset_fragment.avs \
  > tests/regression/data/expected_md5_320x240_seed1234.json
```

The command prints a JSON payload containing the per-frame hashes. The same
snapshot check is wired into CTest via `offscreen_golden_md5_snapshot`, which
invokes the tool and verifies the output JSON matches the committed version.


## build dependencies

`cmake g++ clang-format git pkg-config` 

### Fedora

`SDL2-devel mesa-libGL-devel mesa-libGLU-devel mesa-libGLU-devel portaudio-devel portaudio gtest-devel jack-audio-connection-kit-devel  alsa-lib-devel`

### Debian

`libsdl2-dev mesa-common-dev libglu1-mesa-dev portaudio19-dev libportaudio2 libgtest-dev libjack-dev libasound2-dev`

> **Note:** If your distribution ships `libsdl2-dev` older than 2.28, the
> build will automatically download and build SDL 2.28.5 locally during the
> CMake configure step. Installing a system package `>= 2.28` is still
> preferred when available.


## Resources 
- [https://visbot.net/]
- [https://forums.winamp.com/forum/visualizations/avs]
- [https://visbot.github.io/AVS-Forums/html/f-85.html]
- [https://avs.sh]: 
- [https://avs.sh/collections/APEs/]



## Related Projects 
 
