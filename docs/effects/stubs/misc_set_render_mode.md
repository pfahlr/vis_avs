# Misc / Set render mode

Sets the legacy line rendering override used by effects that rasterise lines, dots, and
superscopes. The override mirrors the behaviour of `R_LineMode` from classic AVS and is
stored in the shared `GlobalState`, allowing multiple effects to read the same blend mode
and thickness configuration.

## Parameters

| Name | Type | Default | Description |
| ---- | ---- | ------- | ----------- |
| `enabled` | bool | `true` | When `false` the effect leaves the previous override untouched. |
| `mode` / `mode_name` / `blend_mode` | int / string | `replace` | Blend operation. Accepts numeric indices (`0`–`9`) or names such as `additive`, `maximum`, `50/50`, `sub1`, `sub2`, `multiply`, `adjustable`, `xor`, and `minimum`. |
| `alpha` / `blend_alpha` / `adjust` / `adjustment` | int | `0` | Adjustable blend amount (`0`–`255`) used when `mode` is set to `adjustable`. |
| `line_width` / `linewidth` / `line_size` / `linesize` / `thickness` | int | `1` | Stroke thickness hint (`1`–`255`). Effects that do not receive an explicit width fall back to this value. |
| `value` / `raw` | int | `0x80010000` | Optional 32-bit packed representation for presets that store the legacy bitfield directly. |

## Behaviour

* The effect writes the packed mode (`mode`, adjustable alpha, and line width) into
  `GlobalState::legacyRender.lineBlendMode` each frame when `enabled` is `true`.
* Rendering primitives consult this shared state when no explicit blend parameters are
  provided, enabling preset authors to coordinate line widths and blend styles across
  multiple effects.
* Modes reproduce the classic AVS semantics: replace, additive, maximum, 50/50, two
  subtractive variants, multiply, adjustable blend, XOR, and minimum.

Effects that support the override automatically honour presets authored for classic AVS
without further changes.
