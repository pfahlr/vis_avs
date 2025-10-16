# Superscope effect

`superscope` embeds the AVS/EEL virtual machine and renders a polyline or point
cloud by executing user-provided scripts. The implementation mirrors the classic
Winamp behaviour so legacy presets continue to work, but runs entirely on the
integer geometry backend for deterministic output.

## Script stages

| Parameter | Runs | Purpose |
|-----------|------|---------|
| `init` | once at `init()` | Allocate globals, set initial colours, override defaults. |
| `frame` | once per rendered frame | Update per-frame state and colours. |
| `beat` | whenever the audio analyser flags a beat | Optional beat reaction. |
| `point` | for each of the `n` points | Emit the final position/colour for a vertex. |

All stages share the same VM state, so globals and `gmegabuf()` persist between
calls. Scripts are compiled lazily and recompiled whenever a parameter changes.

## Available variables

The runtime exposes the familiar Superscope registers:

* `time`, `frame` – seconds since start and integral frame index.
* `bass`, `mid`, `treb` – analyser bands; `rms` – mono RMS amplitude.
* `beat`, `b` – 1.0 when the `beat` stage will run, otherwise 0.0.
* `n` – requested point count (defaults to 512). Writing to `n` clamps to
  `[1, 131072]` and controls the loop in the renderer.
* `i` – current point index normalised to `[0, 1]`.
* `v` – waveform sample at `i`. Supplied for backwards compatibility.
* `w`, `h` – framebuffer width/height in pixels.
* `linesize`, `drawmode` – line thickness (≥1) and draw mode (`0` = dots,
  `1` = connect consecutive points).
* `skip` – skip flag; points with `skip > 0` are not drawn.
* `x`, `y` – current point coordinates in the `[-1, 1]` Superscope space.
* `red`, `green`, `blue` – output colour channels in `[0, 1]`.

Legacy helpers such as `getosc()`, `getspec()`, and `gmegabuf()` are backed by
`avs::EelVm`, with input buffers resampled to match the original 576-sample
layout (including dual-channel copies).

## Overrides

The renderer exposes a few convenience parameters that override script output:

| Parameter | Type | Description |
|-----------|------|-------------|
| `n` | int | Replaces the script-controlled point count. |
| `linesize` | float | Overrides `linesize` before rendering. |
| `drawmode` | bool/int | Forces dot (`0`) vs line (`1`) rendering. |
| `init`, `frame`, `beat`, `point` | string | EEL scripts for each stage. |

Overrides apply after `frame`/`beat` have executed and before iterating points,
so they emulate the behaviour of the classic UI checkboxes.

## Rendering behaviour

The renderer copies the previous framebuffer into the current frame (matching
Superscope's feedback mode), then iterates `n` points:

1. Seeds `i`, `v`, default `x`/`y` (a horizontal line), and colours sampled from
   the previous frame.
2. Executes the `point` script.
3. Converts `x`, `y` from `[-1, 1]` into pixel coordinates.
4. Draws either a thick dot or a segment from the previous point using
   `drawThickLine`, honouring `skip` and `drawmode`.

All arithmetic stays in integers during rasterisation, so repeated runs with the
same scripts and audio data produce byte-for-byte identical output.
