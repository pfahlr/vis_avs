# Core blending and swizzle effects

The modern effects core exposes deterministic CPU implementations of AVS's
classic blending and channel swizzle operations. The `blend`, `overlay`, and
`swizzle` effects are registered via `avs::effects::registerCoreEffects()` and
share the `BlendOp` enum declared in
`libs/avs/effects/include/avs/effects/blend_ops.hpp`.

## Blend effect

`blend` applies a single operation between the framebuffer contents and a
constant foreground color. Parameters are supplied through
`avs::core::ParamBlock` keys:

| Key        | Type    | Default | Description |
|------------|---------|---------|-------------|
| `op`       | string  | `replace` | Blend operation token (case insensitive). |
| `fg` / `foreground` | int | `0x000000` | Foreground RGB color encoded as `0x00RRGGBB`. |
| `fg_alpha` | int | `255` | Optional alpha channel for the foreground color. |
| `alpha`    | int | `255` | Alpha used by the `ALPHA` mode. |
| `alpha2`   | int | `alpha` | Alpha used by the `ALPHA2` mode. |
| `slide` / `blend` | int | `alpha` | Slider value used by `ALPHASLIDE` and `BLENDSLIDE`. |

All parameters are clamped to the 8-bit range. The foreground color is blended
into the existing framebuffer without allocating additional memory. Operations
match the legacy DLL's behaviour:

| Token | Behaviour |
|-------|-----------|
| `replace` | Replace destination with the foreground color. |
| `additive` / `add` | Component-wise saturating addition. |
| `blend` | 50/50 average (integer, rounded). |
| `alphaslide` | Interpolate using the `slide` parameter (`0` → destination, `255` → source). |
| `alpha` | Interpolate using `alpha`. |
| `alpha2` | Interpolate using `alpha2`. |
| `blendslide` | Alias for `alphaslide`. |
| `defaultblend` | Alias for `blend`. |
| `defrendblend` | Alias for `blend` (matches the render-list "default render blend"). |
| `above` | Per-channel maximum. |
| `below` | Per-channel minimum. |

All math is performed with integer arithmetic that mirrors AVS's look-up-table
implementation. Saturation uses branchless masks to preserve SIMD-friendly inner
loops.

## Overlay effect

`overlay` blends two constant colors and writes the result across the entire
framebuffer. Parameters mirror the `blend` effect with additional background
color support:

| Key        | Type    | Default | Description |
|------------|---------|---------|-------------|
| `op`       | string  | `replace` | Blend operation token. |
| `bg` / `background` | int | `0x000000` | Background RGB color. |
| `bg_alpha` | int | `255` | Optional background alpha. |
| `fg` / `foreground` | int | `0x000000` | Overlay RGB color. |
| `fg_alpha` | int | `255` | Optional overlay alpha. |
| `alpha` / `alpha2` / `slide` | int | `255` | Same semantics as for `blend`. |

The operation is evaluated per pixel by first seeding the background color and
then applying the selected `BlendOp`.

## Swizzle effect

`swizzle` permutes RGB channels in-place. The `mode` (or `order`) parameter
accepts the six permutations: `rgb`, `rbg`, `grb`, `gbr`, `brg`, `bgr`. Alpha is
always preserved. The implementation computes the new channel tuple and writes
it back once per pixel to avoid intermediate aliasing.

## Micro preset parser

`parseMicroPreset()` in `libs/avs/effects/src/micro_preset_parser.cpp` provides a
lightweight text format for tests and tooling. Each line specifies an effect key
followed by `key=value` assignments. Tokens that originate from Win32 UI
resource dumps (e.g. `BUTTON1`, `CHECKBOX`, `VIS_*`) are ignored with a warning
and never emit runtime nodes.

Values support decimal and hexadecimal integers (`0x` or `#` prefixes), floats,
and booleans (`true`/`false`, `on`/`off`, `yes`/`no`). Unknown tokens fall back to
strings so presets remain forward compatible.

## Golden tests

`tests/core/test_blend_ops.cpp` drives the blend, overlay, and swizzle effects
through the micro preset parser and validates the resulting framebuffers against
MD5 hashes stored in `tests/golden/micro_blend_md5.txt`. Update the hashes after
behaviour changes with:

```bash
./tools/update_goldens.sh
```

The script rebuilds `core_effects_tests` and reruns it with `UPDATE_GOLDENS=1` so
the golden manifest is rewritten automatically.

