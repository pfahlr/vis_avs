# Render / SVP Loader

The SVP loader bridges legacy Sonique Visualization Plug-ins (`.svp` / `.uvs`) into the new
renderer. When a preset specifies a plug-in library, the effect searches for the DLL, loads it,
and forwards AVS audio data to the plug-in's `Render` callback.

> **Platform support**
> : Windows builds can execute SVP plug-ins directly. Non-Windows builds currently fall back to a
>   silent no-op so presets continue to run.

## Configuration

The loader looks for a string parameter named `library` (falling back to `file`, `path`, or
`filename`). The value can be an absolute path or a relative file name. Relative paths are resolved
against:

* the current working directory
* `resources/` and `resources/svp/` under the application root
* any directories listed in the `VIS_AVS_SVP_PATHS` environment variable (`;`-separated on Windows,
  `:` elsewhere)

If the file name omits an extension the loader automatically probes `.svp` and `.uvs` variants.

## Limitations

* The original plug-in settings API (`OpenSettings` / `SaveSettings`) is not yet integrated.
* Cross-platform execution is limited by the plug-ins themselves; most historical SVP modules ship
  as 32-bit Windows DLLs.
* Framebuffers smaller than the target render surface are ignored to avoid unsafe writes.
