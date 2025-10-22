# Render / SVP Loader

Loads legacy Sonique Visualization Plug-in (`*.svp`/`*.uvs`) modules when available and forwards the
current AVS audio analysis buffers to the plug-in for rendering.  On Windows builds the loader will
attempt to locate a `QueryModule` export inside the selected library, initialize it, and call the
plug-in's `Render` callback each frame.  Other platforms fall back to a no-op so presets that reference
the effect continue to parse without failing.
