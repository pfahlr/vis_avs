# Trans / Blitter Feedback

Renders the previous frame back into the current framebuffer while applying
simple geometric transforms. The effect supports horizontal and vertical
mirroring as well as 90-degree rotations of the feedback image. A feedback
factor (``feedback``/``gain``/``attenuation`` parameters) clamps the maximum
energy of the recursion to avoid runaway brightness.

## Parameters

- `mirror_x` / `flip_horizontal` – mirror the feedback along the X axis.
- `mirror_y` / `flip_vertical` – mirror along the Y axis.
- `rotate_quadrants` / `rotate` – rotate the feedback in 90° steps. Positive
  rotation is clockwise.
- `wrap` – wrap sampling outside the framebuffer instead of clamping.
- `feedback` (`gain`, `attenuation`) – multiplier applied to the sampled RGB
  values, clamped to the `[0, 1]` range.

The `mode` parameter can also combine transformations using tokens such as
`mirror_x`, `mirror_y`, `rotate90`, or `rotate180`.
