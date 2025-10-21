# Render / Dot Plane

The dot plane recreates Winamp AVS's animated grid of audio-reactive points. A 64×64 heightfield is
advected toward the camera, with the frontmost row refreshed from the analyser's waveform and
frequency magnitudes each frame. Per-cell heights feed a colour gradient that interpolates between
five user-defined key colours. The entire lattice tilts around the X axis by `angle` degrees and
rotates around the Y axis according to `rotvel`, matching the legacy module's controls and persisted
rotation offset.

## Parameters

| Key | Type | Default | Notes |
|-----|------|---------|-------|
| `rotvel` | int | `16` | Rotation velocity in legacy slider units (≈0.2°/frame). Negative values spin clockwise. |
| `rotation` | float | `0` | Initial Y-axis rotation in degrees; accepts the saved `r` offset from classic presets. |
| `angle` / `tilt` | int | `-20` | X-axis tilt in degrees, clamped to ±90°. |
| `color0` … `color4` | int | Gradient control points encoded as `0xRRGGBB` values (aliases: `colourN`, `color_N`). |

Audio data comes from `RenderContext::audioAnalysis` when available, falling back to
`RenderContext::audioSpectrum`. The effect performs additive RGB blending without modifying alpha,
mirroring the original `BLEND_LINE` macro.
