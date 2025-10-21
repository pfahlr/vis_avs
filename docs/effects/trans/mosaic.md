# Trans / Mosaic

The **Trans / Mosaic** effect pixelates the framebuffer by sampling a coarse
grid of source pixels and expanding each sample into a block.  It faithfully
reimplements the legacy `R_Mosaic` module using the original fixed-point
integer arithmetic so that existing presets render identically.

## Parameters

| Key | Description |
|-----|-------------|
| `enabled` | When `false` the effect is bypassed. |
| `quality` | Number of samples per axis (1–100).  Lower values produce larger blocks. |
| `quality_onbeat` (`quality2`) | Quality override applied on beat triggers. |
| `on_beat` (`onbeat`) | When `true`, switches to the on-beat quality whenever a beat is detected. |
| `beat_duration` (`durFrames`) | Number of frames the on-beat quality is held after a trigger. |
| `blend` | Enables additive blending with the previous framebuffer instead of a hard overwrite. |
| `blend_avg` (`blendavg`) | Enables 50/50 averaging with the previous framebuffer. |

When both `blend` and `blend_avg` are supplied the additive blend takes
precedence, mirroring the original UI where only one mode could be active at a
time.

## Behaviour

- The effect uses fixed-point stepping (`16.16` precision) to determine which
  source pixel feeds each output block.
- When the effective quality is 100 or higher the framebuffer is left
  untouched.  This matches AVS where quality values above the slider maximum
  disable the mosaic.
- Beat detection latches the on-beat quality for `beat_duration` frames and
  then linearly steps back toward the base quality using integer division,
  reproducing the subtle ramp of the Winamp implementation.

