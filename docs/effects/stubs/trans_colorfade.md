# Trans / Colorfade

Implements the legacy `R_ColorFade` transition. The effect categorises each
pixel by its dominant RGB channel and then adds a configurable offset to all
three colour components. Positive offsets push colours toward white while
negative values pull them toward black. The legacy module exposed three sliders
that control how the offsets rotate across the dominant colour groups; the
modern port keeps the same ordering to remain preset compatible.

## Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `offset_a` (aliases: `offset0`, `offset_r`) | int | `8` | Legacy slider A. Tweaks complementary channels. Clamped to ±32. |
| `offset_b` (aliases: `offset1`, `offset_g`) | int | `-8` | Legacy slider B. Tweaks the dominant channel. Clamped to ±32. |
| `offset_c` (aliases: `offset2`, `offset_b`) | int | `-8` | Legacy slider C. Tweaks the remaining channel. Clamped to ±32. |
| `beat_offset_a/b/c` | int | inherit from base offsets | Overrides used on beat frames when smoothing is enabled and `randomize` is `false`. Each value is clamped to `[-32, 32]`. |
| `flags` | int | `1` | Bitmask matching the original UI: `1` enables, `2` randomises on beats, `4` enables smoothing/beat handling. |
| `enabled` | bool | `true` | Convenience alias for bit 0. Ignored when `flags` is provided. |
| `smooth` / `use_beat_faders` | bool | `false` | Convenience alias for bit 2. When `true`, offsets ease back toward the base values instead of snapping immediately. |
| `randomize` / `randomize_on_beat` | bool | `false` | Convenience alias for bit 1. When smoothing is active, picks fresh random offsets for every beat. |

When smoothing is enabled the per-frame update gently moves the working offsets
back toward the base slider values. Beat events either snap to the
`beat_offset_*` values or generate deterministic random offsets (matching the
original modulo ranges) depending on the `randomize` flag.

The implementation preserves the framebuffer alpha channel and performs
saturating arithmetic on RGB, mirroring the premultiplied blending used by AVS.
