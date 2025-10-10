# Developer Notes

## Built-in Effect Registry

`avs::register_builtin_effects(Registry&)` now registers every core render, transform,
and misc effect. Tests assert that canonical identifiers such as `oscilloscope`,
`movement`, and `clear_screen` are available and produce concrete effect instances.
When adding new effects place the factory wiring in this function so editors and
loaders discover them automatically.

## Pixel Sampling Helpers

`src/core/pixel_ops.cpp` implements the shared CPU sampling helpers exposed from
`avs/core.hpp`:

* `sampleRGBA(view, x, y, options)` clamps or wraps coordinates and supports
  nearest and bilinear filtering.
* `blendPixel(dst, src, mode)` performs standard AVS blend modes and a
  source-over alpha update.

These helpers are used by several effects and have dedicated unit tests in
`tests/test_pixel_ops.cpp`.

## Optional CMake Features

The top-level `CMakeLists.txt` exposes opt-in switches for optional backends and
third-party helpers. All default to `OFF` so the project builds without extra
system dependencies:

```
- DENABLE_SDL3=ON        # Prefer SDL3 instead of SDL2
- DENABLE_PORTAUDIO=ON   # Require a system PortAudio instead of the vendored build
- DENABLE_JACK=ON        # Link against JACK for audio capture/playback
- DENABLE_FFTW3=ON       # Enable FFTW3 support alongside the default FFT
- DENABLE_KISSFFT=ON     # Advertise kissFFT availability to consumers
- DENABLE_FREETYPE=ON    # Link FreeType for future font rendering work
- DENABLE_SDL_TTF=ON     # Link SDL_ttf for font rendering helpers
- DENABLE_STB_IMAGE=ON   # Add stb_image include paths for texture loading
- DENABLE_EEL2=ON        # Build Cockos WDL EEL2 scripting and tests
```

Builds that enable scripting gain the new `avs::EELContext` wrapper defined in
`include/avs/script/EELContext.hpp`. It wraps `EelVm`, exposes variable access,
script compilation/execution, and can be seeded with audio features so
`getosc()/getspec()` behave like Winamp AVS.

## Running Tests

Enable testing when configuring the project and run the suite from the build tree:

```
cmake -B build -S . -DBUILD_TESTING=ON
cmake --build build
cd build && ctest -V
```

This executes the new registry, pixel-helper, and optional EEL context tests in
addition to the existing suites.

