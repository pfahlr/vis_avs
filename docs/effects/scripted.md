# Scripted Effect

The scripted effect executes [EEL2](https://www.reaper.fm/sdk/js/eel2.php)
programs to generate per-frame imagery. It mirrors the classic AVS
Superscope/Color Modifier block with three execution stages:

| Stage  | Invocation                                    |
|--------|-----------------------------------------------|
| `init` | Run once after compilation or preset changes. |
| `frame`| Run once every rendered frame.                 |
| `pixel`| Run for each pixel in the active framebuffer.  |

The engine concatenates the optional `lib` snippet before each stage and
compiles them with the deterministic EEL runtime in
`avs::runtime::script::EelRuntime`. Scripts are sandboxed with a per-frame
bytecode budget to avoid infinite loops. When the budget or compilation fails,
execution continues but the frame receives a red error overlay indicating the
stage that failed.

## Bound variables

All scripts share a single environment. The following scalar variables are
pre-registered:

- `time` – accumulated time in seconds.
- `frame` – zero-based frame index.
- `width`, `height` – output dimensions in pixels.
- `x`, `y` – normalized pixel coordinates in the range `[-1, 1]` with the
  origin at the framebuffer center.
- `red`, `green`, `blue` – current pixel color components in the range `[0, 1]`.
  The pixel stage should write the desired output values here.
- `bass`, `mid`, `treb` – simple averages of the low, mid, and high thirds of
  the supplied spectrum buffer.
- `arbval` – numeric value parsed from the `arbval` parameter.
- `q1` … `q32` – persistent general-purpose registers exposed for compatibility
  and visual debugging.

Any value assigned to these variables is preserved between stages in the same
frame. The register values are also rendered as an overlay in the top-left
corner to aid debugging.

## Rendering behaviour

Each invocation of the pixel stage reads the existing framebuffer pixel,
computes the new RGB values, and writes back clamped `[0, 255]` bytes with an
alpha channel of `255`. Pixels are processed in scanline order and inherit their
previous value unless overwritten by the script.

The runtime uses a deterministic RNG seeded from the pipeline’s
`avs::core::DeterministicRng`, ensuring repeatable behaviour across runs.

## Error reporting

Compilation or execution failures do not abort the pipeline. Instead, a red
text overlay is drawn describing the failing stage (`COMPILE`, `FRAME`, or
`PIXEL`). Register dumps remain visible to aid troubleshooting.

## Example preset fragment

```text
scripted lib="global_scale=10;"
         init="q1=0;"
         frame="q1=q1+0.2; q5=sin(q1);"
         pixel="radius=sqrt(x*x+y*y);\n"
               "angle=atan2(y,x);\n"
               "wave=0.5+0.5*sin(radius*global_scale-q1);\n"
               "red=wave;\n"
               "green=0.5+0.5*sin(angle+q5);\n"
               "blue=0.5+0.5*cos(radius*12+q5);"
```

The snippet animates a radial sine pattern while exposing the current phase in
`q1` and `q5`, which is mirrored in the register overlay.
