# Transform Affine Effect

The `transform_affine` effect draws a test triangle using a configurable affine
transform pipeline. It exists primarily to validate the math used by the
revived AVS renderer and to provide a deterministic harness for exercising
anchor placement and beat gating behaviour.

## Parameters

| Token / key        | Description |
|--------------------|-------------|
| `anchor=<pos>`     | Places the triangle relative to a named anchor.  Valid
  values are `top_left`, `top_right`, `bottom_left`, `bottom_right`, and
  `center`.  When omitted the anchor defaults to the top-left corner. |
| `CENTER`           | Convenience flag equivalent to `anchor=center`. |
| `rotate=<deg>`     | Rotation speed in degrees per frame.  Supplying the bare
  `ROTATE` token enables a default 90°/frame rotation. |
| `angle=<deg>`      | Static angle offset applied before the per-frame
  rotation. |
| `scale=<factor>`   | Scales the canonical triangle before rotation. |
| `2X`               | Doubles the final render size after scaling. |
| `color=<0xRRGGBB>` | Fill colour for the triangle. |
| `test`             | Enables additional debug overlays, notably the anchor
  crosshair rendered in white. |
| `log_rows=<n>`     | Number of rows at the bottom of the framebuffer reserved
  for the gating log (defaults to 1).  Additional rows show progressively
  older history stacked upward. |
| `randompos`        | When used together with beat gating the anchor receives a
  deterministic jitter on each trigger.  The amplitude can be controlled with
  `random_offset=<0..1>` representing a fraction of the framebuffer size. |
| `random_angle=<deg>` | Applies a random angle offset sampled per beat within
  ±`deg`. |
| `random_scale=<factor>` | Applies a random multiplicative scale sampled per
  beat within ±`factor`. |
| `5050`             | Switches the triangle fill to an averaged 50/50 blend
  with the existing pixels instead of a hard overwrite. |

## Visual Aids

When `test` is enabled the effect renders a white crosshair at the active
anchor.  The bottom rows display a history of the gating state using the
following colour key:

- **Red** — Beat trigger this frame.
- **Green** — Hold frame while the on-beat latch counts down.
- **Yellow** — Sticky latch engaged.
- **Dark grey** — Inactive.

These overlays make it straightforward to assert anchor placement and gating
behaviour in automated tests.
