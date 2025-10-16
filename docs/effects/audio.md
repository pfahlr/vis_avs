# Audio Overlay Effects

The audio overlay effects expose deterministic visualizations driven by the
fixed-size audio analyzer implemented in `audio/analyzer.h`. Each effect uses
the same 1024-sample Hann-windowed FFT and produces the following derived
features every frame:

- Waveform trace (576 averaged samples)
- Magnitude spectrum (512 bins)
- Smoothed bass / mid / treble band energies
- Beat and BPM estimates with confidence scoring

## Supported overlay effects

| Effect Key         | Description                                                  |
|--------------------|--------------------------------------------------------------|
| `effect_wave`      | Draws the oscilloscope trace centred in the framebuffer.     |
| `effect_spec`      | Renders a linear bar spectrum with a perceptual colour ramp. |
| `effect_bands`     | Displays three wide bars for bass/mid/treble energy.         |
| `effect_leveltext` | Text readout of the current level, BPM and beat state.       |
| `effect_bandtxt`   | Textual bass/mid/treble figures for debugging or overlays.   |

Register the factories with `avs::effects::registerCoreEffects()` and add the
desired overlay to a `avs::core::Pipeline` using the effect key above.

## Parameters

All overlays accept the following optional parameters (case-insensitive):

- `gain=<float>` – multiplies the analysed magnitude before rendering. Defaults
  to `1.0`.
- `color=#RRGGBB` – base colour for the waveform overlay.
- `text_color=#RRGGBB` – text foreground colour for the textual overlays.
- `damp=<0|1>` – enable exponential smoothing (default `1`). Set to `0` to
  disable damping and use raw analysis values.
- `beat=<0|1>` – for `effect_wave` / `effect_bands` toggles beat highlighting.

Parameters may be supplied either as micro preset entries or via direct
`avs::core::ParamBlock` assignment. Numeric flags (`0`/`1`) and boolean tokens
(`on`/`off`) map to the same fields.

## Determinism and testing

The analyzer is fully deterministic. Tests in `tests/presets/audio_vis/`
render ten frames per overlay and compare the framebuffer hash against stored
golden values. These tests use procedurally generated audio fixtures to avoid
shipping binary WAV assets while keeping predictable phase relationships for
sine and percussive components.
