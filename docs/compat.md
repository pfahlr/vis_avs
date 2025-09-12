# Compatibility Notes

The legacy AVS preset parser currently supports the following effects with
scalar and SSE2 implementations where available:

- Blur
- ColorMap
- Convolution
- MotionBlur
- ColorTransform
- Glow
- ZoomRotate
- Mirror
- Tunnel
- RadialBlur
- AdditiveBlend
- Scripted effect (frame script only)

Unsupported effects are replaced with a no-op placeholder and emit a warning
on load. Unknown fields from the preset are preserved internally for future
round-tripping.

