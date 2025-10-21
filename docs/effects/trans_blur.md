# Trans / Blur

`Trans / Blur` performs a separable box blur on the current framebuffer and
blends the result back with configurable strength.  The implementation lives in
`src/effects/trans/effect_blur.cpp` and mirrors the classic AVS control scheme
while adding modern routing options.

## Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `radius` | int | `1` | Pixel radius for each axis. Values are clamped to the range `[0, 32]`. |
| `strength` | int/float | `256` | Blend factor applied to the blurred result. Integers use the legacy 0–256 scale (256 = fully blurred). Floats in `[0.0, 1.0]` are accepted for convenience. |
| `horizontal` | bool | `true` | Enables the horizontal pass. When disabled the effect only samples vertically. |
| `vertical` | bool | `true` | Enables the vertical pass. When disabled the effect only samples horizontally. |
| `roundmode` | bool | `false` | Matches the legacy "round up" toggle by biasing divisions with half the window size before truncation. |

## Legacy compatibility

Older presets store a single `enabled` field (0 = off, 1 = medium, 2 = light,
3 = heavy).  The loader maps these selectors to the new controls:

- `enabled = 0` → `strength = 0`
- `enabled = 1` → `radius = 1`, `strength = 256`
- `enabled = 2` → `radius = 1`, `strength = 192`
- `enabled = 3` → `radius = 2`, `strength = 256`

Explicit `radius` or `strength` overrides in the preset take precedence over the
legacy mapping, allowing modern presets to fine-tune the blur without breaking
round-tripping for historic content.
