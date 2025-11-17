# Text effect

The `text` effect renders bitmap text into the framebuffer using a deterministic
glyph atlas derived from the public-domain IBM VGA 8×8 font. Rasterisation is
integer-only and therefore fully reproducible across platforms.

## Parameters

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `text` | string | `""` | UTF-8 string (ASCII glyph coverage). Newlines start a new row. |
| `x`, `y` | int | `0` | Anchor position in pixels. |
| `size` / `height` | int | `16` | Glyph height in pixels. |
| `glyphwidth` / `width` | int | auto | Optional glyph width (defaults to aspect ratio). |
| `spacing` | int | `1` | Extra pixels between glyphs/lines. |
| `color`, `alpha` | int | `0xFFFFFF`, `255` | Fill colour and opacity. |
| `halign` | string | `left` | Horizontal alignment: `left`, `center`, `right`. |
| `valign` | string | `top` | Vertical alignment: `top`, `middle`, `bottom`. |
| `antialias` | bool/int | `false` | Enables a post-pass box blur to soften edges. |
| `outlinecolor`, `outlinealpha` | int | `0x000000`, `255` | Stroke colour and opacity. |
| `outlinesize` / `outlinewidth` | int | `0` | Stroke radius in pixels (0 disables). |
| `shadow` | bool/int | `false` | Enables shadow rendering. |
| `shadowcolor`, `shadowalpha` | int | `0x000000`, `128` | Shadow colour and opacity. |
| `shadowoffsetx`, `shadowoffsety` | int | `2` | Pixel offsets applied to the shadow mask. |
| `shadowblur` | int | `2` | Box blur radius for the shadow. |

Booleans accept both literal (`true`/`false`, `on`/`off`) and integer inputs.

## Rendering pipeline

1. **Glyph rasterisation** – characters are scaled to the requested size using
   integer nearest-neighbour sampling. Optional anti-aliasing performs a 6:1
   separable box blur, blended with the original coverage to keep interiors
   opaque.
2. **Padding** – the text bounds expand to accommodate outlines and shadow
   offsets so the stroke never clips.
3. **Outline** – an eight-direction dilation emulates the classic AVS outline
   pass. The resulting mask excludes the interior to avoid double blending.
4. **Shadow** – the base mask is offset, optionally blurred, and blended before
   the stroke and fill, matching Win32's drop-shadow behaviour.
5. **Fill** – the original glyph coverage is blended last using the configured
   colour and opacity.

Regression coverage lives in `tests/core/test_primitives.cpp` and exercises the
anti-alias toggle as well as outline/shadow layering.

