# Affine Transform Effect

The affine transform effect renders a diagnostic triangle that can be rotated,
scaled, and translated around configurable anchors. A full 3×3 matrix is
assembled internally from the `rotation_deg`, `scale_*`, and `translate_*`
parameters before being applied to a base triangle aligned in local space.

## Parameters

| Name | Type | Description |
| ---- | ---- | ----------- |
| `rotation_deg` | float | Rotation applied around the active anchor in degrees. |
| `scale_x`, `scale_y` | float | Independent X/Y scaling factors applied before rotation. |
| `double_size` | bool | Doubles both scale axes to emulate the classic “2X” toggle. |
| `translate_x`, `translate_y` | float | Pixel offsets added after rotation; they are combined with any beat-driven random offsets. |
| `anchor` | string | Anchor selector: `center`, `top_left`, `top_right`, `bottom_left`, `bottom_right`, or `custom`. |
| `anchor_x`, `anchor_y` | float | Normalised 0–1 coordinates used when `anchor=custom`. |
| `use_random_offset` | bool | Adds the gate-provided random offset (see gating effect) to the translation step. |
| `draw_shape` | bool | When false the effect only clears the buffer (useful for starfield-only passes). |
| `crosshair` | bool | Forces anchor crosshairs even when the engine is not running in test mode. |
| `star_count` | int | Optional deterministic starfield overlay; generated using the frame RNG seeded from `VIS_AVS_SEED`. |

Crosshairs are automatically drawn when the engine enters `testMode`, allowing
visual verification of anchor placement across presets. The `lastTriangle()` and
`lastAnchor()` helpers expose the computed geometry so tests can assert precise
positions.

The starfield helper honours `DeterministicRng`, meaning multiple runs with the
same `VIS_AVS_SEED` and frame index produce identical pixel layouts.
