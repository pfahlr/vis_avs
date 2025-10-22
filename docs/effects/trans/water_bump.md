# Trans / Water Bump

Combines a heightmap-based ripple simulator with per-pixel displacement to
approximate refraction over a disturbed water surface. The legacy AVS module
`R_WaterBump` inspired this implementation.

## Behaviour

The effect maintains two integer heightmaps that are updated every frame using a
finite-difference approximation of wave propagation. The active heightmap is
used to derive a normal-like gradient for each pixel. The gradient offsets the
sample position from the previous framebuffer, creating the familiar "water
bump" warping.

On beat events the effect optionally spawns a sine-shaped droplet that disturbs
the heightmap. The droplet can either be positioned deterministically (left,
centre, right / top, middle, bottom) or randomly within the frame bounds.

## Parameters

| Name              | Type   | Default | Description |
|-------------------|--------|---------|-------------|
| `enabled`         | bool   | `true`  | Master enable for the effect. |
| `density` / `damp`| int    | `6`     | Wave damping factor (higher values damp faster). |
| `depth`           | int    | `600`   | Height of beat-triggered droplets. Larger values create stronger ripples. |
| `random_drop`     | bool   | `false` | When `true`, droplets spawn at random positions using the configured radius as a percentage of the larger framebuffer dimension. |
| `drop_position_x` | int    | `1`     | Horizontal droplet anchor when `random_drop` is `false` (`0`=left, `1`=centre, `2`=right). |
| `drop_position_y` | int    | `1`     | Vertical droplet anchor (`0`=top, `1`=middle, `2`=bottom). |
| `drop_radius`     | int    | `40`    | Droplet radius in pixels (or percent when `random_drop` is `true`). |
| `method`          | int    | `0`     | Selects the wave solver (`0`=standard 8-neighbour ripple, `1`=sludge variant). |

Beat detection is driven by `RenderContext::audioBeat`. When the beat flag is
set, the effect spawns one droplet using the configuration above.

## Notes

* Heightmaps automatically resize when the framebuffer dimensions change.
* Edges are clamped to avoid sampling outside the framebuffer.
* The implementation preserves determinism by sourcing randomness from the
  frame-level `DeterministicRng` provided in the render context.
