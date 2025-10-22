# Render / SVP Loader

The SVP loader effect bridges legacy Sonique Visualization Plug-in (SVP/UVS) modules
with the modern AVS runtime. At runtime the effect can attempt to load a
platform-native dynamic library, query its `QueryModule` export, and forward
render calls with the expected waveform and spectrum buffers.

* On supported platforms (Windows), the effect will try to open `.svp`, `.uvs`
  or `.dll` files when a `library` parameter is provided. The loader converts the
  engine's RGBA framebuffer into the ABGR layout that SVP modules expect,
  executes the plug-in renderer, and maps the output back to RGBA.
* On unsupported platforms the loader degrades gracefully without drawing.

SVP plug-ins can request waveform or spectrum data through the `lRequired` flag.
The effect fulfils those requests using the audio analysis supplied by the AVS
runtime.
