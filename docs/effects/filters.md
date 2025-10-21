# Filter effects

This module documents the deterministic post-processing filters implemented in
`src/effects/filters`. Each effect exposes a modern preset key (`filter_*`) and
aliases mirroring their legacy AVS names where applicable.

All effects operate on the RGBA framebuffer stored in
`avs::core::RenderContext::framebuffer`. Alpha channels are preserved unless
explicitly noted.

## `filter_blur_box` (alias: `Trans / Blur`)

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `radius` | int | `1` | Radius of the box filter in pixels. Values are clamped to the range `[0, 32]`. |
| `preserve_alpha` | bool | `true` | When enabled, the original alpha channel is copied back after blurring the RGB channels. |

The implementation performs a separable, edge-clamped box blur. The effect uses
prefix-sum windows to avoid per-tap branching while ensuring deterministic
results across platforms.

## `filter_grain` (alias: `Trans / Grain`)

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `amount` | int | `16` | Maximum absolute noise offset applied per channel. Clamped to `[0, 255]`. |
| `seed` | int | `0` | Additional seed mixed with `VIS_AVS_SEED` for reproducible patterns. |
| `monochrome` | bool | `false` | Generates a single noise value per pixel and applies it to all RGB channels when set. |
| `static` | bool | `false` | Reuses a cached noise pattern generated from the base seed instead of regenerating each frame. |

Noise is added with saturating arithmetic. Dynamic mode derives a deterministic
per-frame `std::mt19937` from the seeded RNG, while static mode caches the
pattern for the active frame size.

## `filter_interferences` (alias: `Trans / Interferences`)

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `amplitude` | int | `64` | Peak modulation applied to the tinted stripes. |
| `period` | int | `8` | Stripe period in pixels. Values smaller than `1` are clamped to `1`. |
| `speed` | int | `1` | Phase advance per frame. |
| `phase` | int | `0` | Initial stripe offset. |
| `noise` | int | `16` | Optional additive jitter per sample. |
| `tint` | int | `0xFFFFFF` | Tint color (`0xRRGGBB`) used to weight the channel deltas. |
| `mode` | string | `"add"` | Combination mode: `add`, `subtract`, or `multiply`. |
| `vertical` | bool | `false` | Swaps the stripe orientation to vertical when set. |

A weighted sine blend drives the banding pattern. Optional noise jitter is
constructed from the deterministic RNG and only consumes a single random sample
per frame.

## `filter_fast_brightness` (alias: `Trans / Fast Brightness`)

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `amount` | float | `2.0` (or `0.5` when `mode == 1`) | Scalar gain applied to RGB channels. |
| `mode` | int | `0` | Legacy compatibility: `0` doubles brightness, `1` halves it. Ignored when `amount` is provided. |
| `bias` | float | `0.0` | Linear offset applied after scaling. |
| `clamp` | bool | `true` | When `true`, saturates the result to `[0, 255]` before quantisation. When `false`, stores the 8-bit truncation of the computed value (wrapping on overflow). |

The effect multiplies each color channel by the requested gain, optionally adds
an offset, and stores the rounded result. With clamping disabled the final
assignment follows 8-bit wraparound semantics, matching the original AVS plug-
in. When both `amount` equals `1.0` and `bias` equals `0.0`, the pass is
skipped.

## `filter_unique_tone` (alias: `Trans / Unique tone`)

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `color` | int | `0xFFFFFF` | Target tint in `0xRRGGBB` format. |
| `color_r` / `color_g` / `color_b` | int | derived from `color` | Override individual RGB components (0–255). |
| `invert` | bool | `false` | Flips the sampled luminance before mapping it to the tone. |
| `mode` / `blend_mode` | string | `"replace"` | Blending strategy: `replace`, `add`/`additive`, or `avg`/`average`/`5050`. |
| `blend` | bool | `false` | Legacy toggle for additive mode. Takes precedence over `mode` when provided. |
| `blendavg` | bool | `false` | Legacy toggle for 50/50 averaging mode. |
| `enabled` | bool | `true` | Bypasses the effect when set to `false`. |

The effect collapses each pixel to a single tone while preserving its relative
luminance. A linear RGB luma estimate (`0.2126·R + 0.7152·G + 0.0722·B`) drives
the tone intensity. The computed brightness is scaled along the chosen tint,
ensuring neutral whites remain white while coloured tints keep their perceived
brightness. Additive and averaged blend modes mirror the legacy behaviour from
the original plug-in.

## `filter_color_map`

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `table`/`map` | string | identity ramp | Whitespace-separated list of `RRGGBB` or `AARRGGBB` values (256 entries recommended). Missing entries repeat the last parsed color. |
| `channel` | string | `"luma"` | Source channel used as the lookup index: `luma`, `red`, `green`, `blue`, or `alpha`. |
| `invert` | bool | `false` | Flips the 0–255 lookup index before sampling. |
| `map_alpha` | bool | `false` | Replaces the destination alpha with the table’s alpha component when set. |

A 256-entry lookup table maps input intensity to output color. Luma indices use
integer NTSC-style weights `(0.2126, 0.7152, 0.0722)` approximated by
`(54, 183, 19)`.

## `filter_conv3x3`

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `kernel`/`matrix` | string | identity kernel | Nine whitespace-separated coefficients laid out row-wise. Missing entries default to zero. |
| `divisor` | float | sum of coefficients | Divides the accumulated value. If omitted or near zero, the kernel sum is used, falling back to `1`. |
| `bias` | float | `0.0` | Offset added after division. |
| `clamp` | bool | `true` | Clamps each channel to `[0, 255]` before quantisation. |
| `preserve_alpha` | bool | `true` | Copies the source alpha instead of filtering it. |

The convolution operates on RGB channels (and alpha when requested) with
edge-clamped sampling. Intermediate calculations use single-precision floats to
match the behaviour of the original plug-in while remaining deterministic.

