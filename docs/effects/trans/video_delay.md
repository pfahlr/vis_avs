# Holden04: Video Delay

The Holden04 video delay effect replays frames from a circular history buffer.
It reproduces the timing behaviour of the original AVS APE while preserving the
stored RGBA data so downstream passes can blend it according to the legacy
alpha semantics.

## Parameters

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `enabled` | bool | `true` | Disables the effect when `false`. History is preserved while disabled. |
| `use_beats` | bool | `false` | When `true`, the delay is driven by the number of frames since the last detected beat multiplied by `delay`. |
| `delay` | int | `10` | In frame mode this is the fixed number of frames to rewind (clamped to `[0, 200]`). In beat mode it acts as a multiplier clamped to `[0, 16]`. |

When running in beat mode the delay is recomputed on each beat trigger and
clamped to at most 400 frames, matching the original plug-in. Frames are stored
verbatim (including alpha) so partially transparent content can be composed by
subsequent stages exactly like the legacy module.
