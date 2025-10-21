# Render / AVI

The modern renderer does not yet ship with a cross-platform AVI decoder. The
`Render / AVI` module therefore displays either a static thumbnail extracted
from a PNG/JPEG companion file or a placeholder card to indicate that the
original video could not be loaded.

Parameters recognised by the compatibility layer:

- `enabled` – disable the effect entirely when `false`.
- `source` – original AVI path, used to look up companion thumbnails.
- `fallback_image`/`thumbnail` – optional explicit path to a PNG or JPEG image
  drawn instead of the video.

AVI playback will be implemented once the runtime bundles the required video
backend. Until then presets continue to render deterministically with the
placeholder output.
