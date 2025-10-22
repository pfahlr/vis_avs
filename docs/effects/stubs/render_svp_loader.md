# Render / SVP Loader

The SVP Loader bridges legacy Sonique Visualization Plug-in (SVP/UVS) modules with the
modern renderer.  When a preset specifies a library, the loader attempts to resolve the
path, dynamically load the module and invoke its `Render` entry point each frame.  On
platforms that do not support dynamic libraries, or when a plug-in cannot be found, the
loader safely falls back to a no-op so the preset chain continues to run.

## Parameters

- `library` / `path` / `file`: Preferred name or path to the SVP/UVS module.  Relative
  paths are resolved against the current working directory and `resources/svp`.

## Behaviour

- Audio waveform and spectrum data are converted to the byte-oriented buffers expected by
  SVP plug-ins.
- Plug-ins receive a 32-bit BGRA framebuffer matching the current render target.
- Settings are loaded from and saved to `avs.ini` placed alongside the selected module, if
  the plug-in exposes `OpenSettings`/`SaveSettings` callbacks.
- When loading fails the effect behaves as a pass-through renderer.

> **Note:** Official SVP/UVS modules were shipped as Windows DLLs renamed to `.svp` or
> `.uvs`.  They can only be executed on platforms that support the corresponding binary
> format.
