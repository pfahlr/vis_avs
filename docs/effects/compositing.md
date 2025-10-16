# Compositing and image pipeline effects

## Effect List

`effect_list` now parses nested JSON-style configurations and instantiates child
effects directly from the global registry. The `config` parameter accepts a
string containing either a single object or an array of objects using the
following schema:

```json
[
  {"effect": "picture", "children": []},
  {"effect": "mirror", "children": []}
]
```

Nested lists are expressed by specifying `children` for any node whose
`effect` resolves to another `effect_list`. Child lists inherit the factory that
produced their parent, so callers only need to register the built-in catalog
once. The list executes its children sequentially in a single pass, re-running
`init()` on any child created after the parent has already been initialised.

## Mirror

The transform mirror effect exposes a new `mode` select parameter with the
classic AVS options:

- `horizontal` – reflect the framebuffer across the vertical axis.
- `vertical` – reflect the framebuffer across the horizontal axis.
- `quad` – mirror both axes so all quadrants display a reflected version of the
  top-left.

Processing is in-place; the implementation snapshots the input rows before
writing to avoid read-after-write artefacts.

## Interleave

`interleave` keeps a history of the most recent frames and interleaves them in a
checkerboard pattern. Two parameters are available:

- `frame_count` (minimum 1) – number of frames to keep in the history.
- `offset` – integer shift applied to the modulo selection per frame.

For each pixel the effect selects a history slot using
`(frame_index + offset + (x + y) % frame_count) % frame_count` while falling
back to the current frame when a slot has not been populated yet. The
`RuntimeCompositing.InterleaveProducesCheckerboard` unit test verifies the
behaviour for a two-frame alternating pattern.

## Picture

The picture renderer now uses `stb_image` to load PNG/JPEG assets into a local
RGBA cache. The `path` string parameter points at the file to load. The image is
blitted to the upper-left corner of the destination framebuffer without scaling
and caches the decoded pixels so subsequent frames avoid disk I/O. The
`RuntimeCompositing.PictureEffectMirrorsHorizontally` test demonstrates loading
a temporary PNG and mirroring it horizontally.
