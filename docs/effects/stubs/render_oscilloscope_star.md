# Render / Oscilloscope Star

Renders five radial spokes whose thickness is modulated by the incoming audio
waveform. Each arm samples the 576-point mono waveform provided by the audio
analyzer and bends the stroke according to the instantaneous amplitude. The
arms rotate continuously, producing the classic "oscilloscope star" shape from
the original AVS module.

## Parameters

| Key | Type | Description |
| --- | ---- | ----------- |
| `size` | int | Scale factor (0–32) that controls the overall star radius. |
| `rotation` / `rotation_speed` | float | Rotation speed in legacy units (positive values rotate clockwise). |
| `position` | string/int | Anchor point along the horizontal axis: `left`, `center`, or `right`. |
| `channel` | string/int | Reserved for compatibility with the legacy module. Mono waveforms treat all channels identically. |
| `color`, `color0..15`, `color1..16` | int/string | Palette entries encoded as `0xRRGGBB` or `#RRGGBB`. Colours are interpolated frame-to-frame. |

All colours are blended smoothly, cycling through the supplied palette at the
same cadence as the original plug-in (64 frames per colour transition).
