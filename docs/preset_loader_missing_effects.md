# Preset loader decoding coverage

The binary preset loader currently recognizes only a small subset of the
effect indices defined in `effectNameForId` inside
`libs/avs-core/src/preset.cpp`. Specifically, it can parse the following
effect payloads:

- **21 – Misc / Comment**
- **45 – Trans / Color Modifier**
- **03 – Trans / Fadeout**
- **06 – Trans / Blur**
- **26 – Trans / Mirror**

All other effect indices are presently treated as unknown and trigger the
"preset loader does not yet decode effect" warning that appears when loading
presets. The table below enumerates every remaining index so implementers can
track decoding progress.

| Effect ID | Effect label |
|----------:|:-------------|
| 0 | Render / Simple |
| 1 | Render / Dot Plane |
| 2 | Render / Oscilliscope Star |
| 4 | Trans / Blitter Feedback |
| 5 | Render / OnBeat Clear |
| 7 | Render / Bass Spin |
| 8 | Render / Moving Particle |
| 9 | Trans / Roto Blitter |
| 10 | Render / SVP Loader |
| 11 | Trans / Colorfade |
| 12 | Trans / Color Clip |
| 13 | Render / Rotating Stars |
| 14 | Render / Ring |
| 15 | Trans / Movement |
| 16 | Trans / Scatter |
| 17 | Render / Dot Grid |
| 18 | Misc / Buffer Save |
| 19 | Render / Dot Fountain |
| 20 | Trans / Water |
| 22 | Trans / Brightness |
| 23 | Trans / Interleave |
| 24 | Trans / Grain |
| 25 | Render / Clear screen |
| 27 | Render / Starfield |
| 28 | Render / Text |
| 29 | Trans / Bump |
| 30 | Trans / Mosaic |
| 31 | Trans / Water Bump |
| 32 | Render / AVI |
| 33 | Misc / Custom BPM |
| 34 | Render / Picture |
| 35 | Trans / Dynamic Distance Modifier |
| 36 | Render / SuperScope |
| 37 | Trans / Invert |
| 38 | Trans / Unique tone |
| 39 | Render / Timescope |
| 40 | Misc / Set render mode |
| 41 | Trans / Interferences |
| 42 | Trans / Dynamic Shift |
| 43 | Trans / Dynamic Movement |
| 44 | Trans / Fast Brightness |

In addition, list containers (effect ID `0xFFFFFFFE`) are parsed and expanded
recursively, so no action is required for them.

