# Render / Moving Particle

Implements the legacy AVS "Moving Particle" renderer. The effect animates a single particle using a
critically damped spring that chases a beat-driven target. Each frame renders a soft disk (or a single
pixel for small radii) using legacy blend modes so presets retain the classic motion trail and bounce
behaviour.

## Parameters

| Name | Type | Default | Description |
| ---- | ---- | ------- | ----------- |
| `enabled` | bool/int | `true`/`1` | Master enable. When expressed as an integer the low bit enables rendering and bit 1 enables beat pulsing. |
| `beat_pulse` | bool | `false` | If true, the trail radius snaps to `size_beat` whenever a beat fires before relaxing back to `size` over subsequent frames. |
| `color` / `colors` | int | `0xFFFFFF` | Particle tint in `0xRRGGBB` form (legacy presets use `RGB()` ordering). |
| `max_distance` / `maxdist` | int | `16` | Scales the particle’s travel radius. Larger values let the particle reach further from screen centre. |
| `size` / `size_base` | int | `8` | Resting radius of the rendered disk. |
| `size2` / `size_beat` | int | `8` | Beat pulse radius. |
| `blend` / `blend_mode` | int/string | `add` | Blending strategy: `replace`, `add`, `average`, or `line` (honours the line mode blend settings). |
| `line_blend_mode` | int | `0` | When `blend_mode` is `line`, selects the legacy line blend operation (0–9). |
| `line_blend_amount` | int | `128` | Weight used by the adjustable line blend (`mode 7`). |

The effect consumes audio beat events from the render context and uses the deterministic per-frame RNG
for target jitter. Rendering succeeds even if no framebuffer is available, matching the defensive
behaviour of the original module.
