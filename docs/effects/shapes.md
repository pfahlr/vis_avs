# Shapes effect

`shapes` draws simple vector primitives entirely on the CPU using the integer
raster utilities introduced for the geometry module. The output is completely
deterministic and therefore safe for snapshot-based testing.

## Supported primitives

* **Circle** – filled disc plus optional outline.
* **Rect** – axis-aligned rectangle centred on `(x, y)`.
* **Star** – regular star polygon with alternating inner/outer radii.
* **Line** – thick line segment from `(x, y)` to `(x2, y2)` (defaults to
  `(x + radius, y)` when no endpoint is supplied).

## Parameters

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `shape` / `type` | select | `circle` | Primitive type: `circle`, `rect`, `star`, `line`. |
| `x`, `y` | int | `0` | Centre (or start point for lines). |
| `radius` | int | `50` | Circle radius, star outer radius, or default line extent. |
| `width`, `height` | int | `100` | Rectangle width/height in pixels. |
| `inner_radius` | int | half of `radius` | Star inner radius (clamped ≥ 1). |
| `points` | int | `5` | Number of star points (minimum 3). |
| `rotation` | float | `0` | Star rotation in degrees (clockwise, 0° at +X axis). |
| `filled` | bool/int | `true` | Enables fill pass. `false` renders outline only. |
| `color`, `alpha` | color/int | white/`255` | Fill colour and opacity. |
| `outlinecolor`, `outlinealpha` | color/int | black/`255` | Outline colour and opacity. |
| `outlinewidth` / `outlinesize` | int | `0` | Outline thickness in pixels (0 disables). |
| `x2`, `y2` | int | start point | Explicit end coordinate for line mode. |

All boolean parameters accept numeric (`0`/`1`) and textual (`true`/`false`,
`on`/`off`) forms.

The renderer first draws the fill primitive (if enabled) and then strokes the
outline using `drawThickLine`/`strokePolygon`. Strokes respect alpha and blend
on top of the fill so transparent fills can be outlined cleanly.
