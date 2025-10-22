# Trans / Unique tone

Implements the legacy Winamp AVS "Trans / Unique tone" effect. The effect flattens the framebuffer to a
single color tone while preserving the per-pixel luminance (the maximum of the red, green, and blue
channels). Options allow additive or average blending with the existing pixels, as well as luminance
inversion for a negative-style result.
