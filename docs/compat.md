# Compatibility Notes

The legacy AVS preset parser currently supports a minimal subset of effects:

- Blur
- ColorMap
- Convolution
- Scripted effect (frame script only)

Unsupported effects are replaced with a no-op placeholder and emit a warning
on load. Unknown fields from the preset are preserved internally for future
round-tripping.

