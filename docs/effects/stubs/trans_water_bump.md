# Trans / Water Bump

Implements a dynamic water surface simulation that blends ripple-based displacement
with bump-mapped lighting. Incoming pixels are displaced based on a simulated
heightmap while per-pixel normals are derived to apply directional shading for a
glossy, water-like appearance. Beats can inject new ripples either at fixed
positions or randomised locations, mirroring the behaviour of the original
`R_WaterBump` effect.
