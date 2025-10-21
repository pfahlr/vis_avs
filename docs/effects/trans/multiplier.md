# Trans / Multiplier

The **Multiplier** effect rescales the red, green, and blue channels of every
pixel in the framebuffer.  It reproduces the behaviour of the classic AVS
"Trans / Multiplier" component while also allowing finer control through
per-channel parameters.

## Parameters

| Key | Type | Description |
|-----|------|-------------|
| `mode` | int | Selects one of the legacy presets.  The mapping matches the original AVS module: `0` – infinity (any non-black pixel becomes white), `1` – ×8, `2` – ×4, `3` – ×2, `4` – ×½, `5` – ×¼, `6` – ×⅛, `7` – zero (everything except pure white becomes black).  The default is `3` (×2). |
| `factor` | float | Optional uniform multiplier applied to all three colour channels.  When supplied it overrides `mode`. |
| `factor_r` | float | Multiplier for the red channel.  Overrides the legacy mode alongside the other channel factors. |
| `factor_g` | float | Multiplier for the green channel. |
| `factor_b` | float | Multiplier for the blue channel. |

The output is always clamped to the 0–255 range to avoid overflow artefacts.
Alpha data is preserved.

## Notes

- Legacy "infinity" (`mode = 0`) forces any non-zero colour to full white while
  leaving black pixels untouched.
- Legacy "zero" (`mode = 7`) clears every pixel that is not already full white.
- When `factor`, `factor_r`, `factor_g`, or `factor_b` are provided the effect
  switches to continuous scaling instead of the discrete legacy modes.
