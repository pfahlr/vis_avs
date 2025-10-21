# Render / Dot Fountain

The Dot Fountain renderer emits short-lived particles from the center of the
screen and lets them arc outward while the scene slowly rotates. The motion of
the particles is driven by the audio spectrum: louder bins launch dots with
more velocity and brighter colors. A beat pulse adds an extra burst of energy
so the fountain swells when strong transients are detected.

## Parameters

| Parameter | Description |
|-----------|-------------|
| `rotvel` or `rotation_velocity` | Horizontal rotation speed in degrees per frame divided by five. |
| `angle` | Tilt angle (in degrees) applied before rendering the fountain. |
| `rotation` | Optional absolute rotation angle to initialize the effect. |
| `color0` … `color4` | Gradient control points (RGB, `0xRRGGBB`) used to build the 64-entry color table. |

All parameters are optional; sensible defaults are supplied to match the
classic AVS preset.

## Behaviour

The implementation mirrors the legacy Winamp AVS module:

* Thirty spokes are simulated across 256 vertical slices. Each frame the points
  climb upwards with a slight gravity bias while the radius expands.
* New particles are spawned every frame using the corresponding spectrum bin and
  beat flag from the render context.
* Colors are interpolated between the five palette entries to match the original
  gradient table.
* Rendering uses additive blending so dots accumulate brightness when they
  overlap.
