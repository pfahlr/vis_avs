# Dynamic Transform Effects

This module implements a set of deterministic transform effects inspired by the
classic AVS dynamic movement operators. All effects operate on the previous
frame, apply a motion transform, and write the result into the current
framebuffer. Implementations live under `src/effects/dynamic/` and share the
`FrameWarpEffect` helper for bilinear history sampling.

## Script-driven effects

### `dyn_movement`
* Uses the EEL2-compatible runtime to execute a per-frame and per-pixel script.
* Provides `x`, `y`, `orig_x`, `orig_y`, `d`, `angle`, `dx`, `dy`, `frame`,
  `time`, `bass`, `mid`, and `treb` variables, plus 32 persistent `q` registers.
* Per-pixel scripts modify `x`/`y` directly to pick the sample coordinate.
* Audio bindings read from the shared `avs::audio::Analysis` state; `bass`,
  `mid`, and `treb` expose the current band energies.

### `dyn_distance`
* Shares the same runtime bindings but resolves the output coordinate from the
  polar values `d` (radius) and `angle`. Scripts can animate radial motion
  while leaving angular position unchanged.

### `dyn_shift`
* Uses `orig_x`/`orig_y` as the baseline coordinate and adds script-provided
  `dx`/`dy` offsets, making it convenient to implement wave-like or per-axis
  displacements.

The dynamic shader base increases the instruction budget to `4,000,000`
bytes per frame to accommodate high-resolution presets, ensuring parity with
AVS behaviour even when trig-heavy scripts are executed per pixel.

## Parametric transforms

### `movement`
* A matrix-based affine transform driven by param block values.
* Supports scale, rotation (degrees), and normalized offsets (`offset_x`,
  `offset_y`).
* Wrap sampling can be enabled via the `wrap` boolean.

### `zoom_rotate`
* Provides zoom and rotation around a configurable anchor point.
* Anchor coordinates are normalized (0..1) with `(0.5, 0.5)` representing the
  screen centre.
* The zoom factor behaves like AVS: values greater than 1.0 zoom in.

## Tests

`tests/presets/dynamic/test_dynamic.cpp` exercises the runtime by rendering five
frames per effect and comparing MD5 hashes against goldens stored in
`tests/presets/dynamic/golden/*.md5`.
