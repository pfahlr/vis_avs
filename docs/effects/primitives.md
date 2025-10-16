# Primitive raster effects

The primitives module exposes a small collection of deterministic CPU
rasterisers that mirror the behaviour of AVS's classic render components.
They operate on the generic `avs::core::RenderContext` and can be driven via
`avs::core::ParamBlock` or through the micro preset parser.

## Solid

`solid` fills an axis-aligned rectangle with a constant colour. Coordinates are
specified in pixels relative to the framebuffer origin (top-left). Supported
parameters:

| Key | Type | Description |
|-----|------|-------------|
| `x1`, `y1`, `x2`, `y2` | int | Opposing rectangle corners. Aliases: `left`, `top`, `right`, `bottom`. |
| `color` | int | RGB encoded as `0x00RRGGBB`. |
| `alpha` | int | Optional opacity (0–255). |

## Dots

`dot`/`dots` draws filled circles. Points can be supplied as a whitespace or
comma separated list (e.g. `"10,10 40,20"`) or via individual `x`, `y`
integers.

| Key | Type | Description |
|-----|------|-------------|
| `points` | string | Sequence of coordinate pairs. |
| `x`, `y` | int | Single dot position (if `points` omitted). |
| `radius` / `size` | int | Circle radius in pixels. |
| `color`, `alpha` | int | Fill colour and optional opacity. |

## Lines

`line`/`lines` renders a polyline through the provided coordinates. Adjacent
pairs form segments; setting `closed=true` joins the last point to the first.

| Key | Type | Description |
|-----|------|-------------|
| `points` | string | Sequence of coordinates (`"x1,y1 x2,y2 ..."`). |
| `x1`, `y1`, `x2`, `y2` | int | Convenience setters for a single segment. |
| `width` / `thickness` | int | Stroke width in pixels (≥1). |
| `color`, `alpha` | int | Line colour and optional opacity. |
| `closed` | bool/int | Close the polyline. |

## Triangles

`tri`, `triangle`, and `triangles` accept vertices in groups of three. Each
triangle can be filled and optionally outlined.

| Key | Type | Description |
|-----|------|-------------|
| `triangles` / `points` | string | Coordinates (`"x0,y0 x1,y1 x2,y2 ..."`). |
| `x1`..`y3` | int | Direct vertex assignment. |
| `filled` | bool/int | Enable interior fill (default `true`). |
| `color`, `alpha` | int | Fill colour and opacity. |
| `outlinecolor`, `outlinealpha` | int | Stroke colour and opacity. |
| `outlinesize` / `outlinewidth` | int | Stroke thickness in pixels. |

## Rounded rectangles

`rrect`/`roundedrect` draws rounded rectangles with optional fill and outline.

| Key | Type | Description |
|-----|------|-------------|
| `x`, `y` | int | Top-left corner. |
| `width`, `height` | int | Dimensions in pixels. Aliases: `x2`, `y2` for opposite corner. |
| `radius` / `round` | int | Corner radius (clamped to half the width/height). |
| `filled` | bool/int | Fill interior (default `true`). |
| `color`, `alpha` | int | Fill colour and opacity. |
| `outlinecolor`, `outlinealpha` | int | Stroke colour and opacity. |
| `outlinesize`, `outlinewidth` | int | Stroke thickness. |

All primitives rely solely on integer stepping (Bresenham-style) to guarantee
deterministic results. Tests live in `tests/core/test_primitives.cpp`.

