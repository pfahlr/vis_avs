# Bump displacement effect

`bump` applies a screen-space displacement using either the current framebuffer
luminance or a named heightmap stored in `avs::runtime::GlobalState`. The effect
is registered by `avs::effects::registerCoreEffects()`.

Parameters (`avs::core::ParamBlock` keys):

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `scale_x` | float | `0.0` | Horizontal displacement in pixels applied to `(height - midpoint)`. |
| `scale_y` | float | `0.0` | Vertical displacement in pixels applied to `(height - midpoint)`. |
| `midpoint` | float | `0.5` | Height value that produces zero displacement. |
| `use_frame_heightmap` | bool | `true` | When `true`, the effect computes height from the incoming framebuffer's RGB average. |
| `heightmap` | string | `""` | Key of a pre-populated `avs::runtime::Heightmap` (ignored when `use_frame_heightmap` is `true`). |
When sampling the framebuffer the effect copies the source pixels before
displacement to avoid feedback artifacts. External heightmaps are bilinearly
sampled using the pixel centre normalized into the heightmap's resolution.

Example: warp the scene horizontally using a sine-wave heightmap prepared by a
previous step:

```cpp
avs::runtime::Heightmap map;
map.width = width;
map.height = height;
map.samples = generateWave(width, height);
globals.heightmaps["wave"] = map;

avs::core::ParamBlock bump;
bump.setBool("use_frame_heightmap", false);
bump.setString("heightmap", "wave");
bump.setFloat("scale_x", 8.0f);
pipeline.add("bump", bump);
```

The resulting render shifts pixels left/right depending on the normalized
height, enabling faux water ripples or beat-synced distortion without resorting
to per-pixel shaders.

