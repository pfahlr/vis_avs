# Misc / Custom BPM

The **Custom BPM** effect overrides the beat pulses supplied by the audio analyzer
and exposes a configurable gating signal for downstream effects.  Legacy AVS
presets used it to pin the beat detector to a predictable tempo or to thin out
an over-eager beat stream.  The modern implementation keeps the behaviour while
integrating with the shared gating helpers and the globals register bridge.

## Parameters

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `enabled` | bool | `true` | Toggles the effect.  When disabled the upstream beat is passed through unchanged. |
| `arbitrary` | bool | `true` | Generates beats at a fixed tempo instead of using the analyzer.  Takes precedence over `skip` and `invert`. |
| `bpm` | float | `120.0` | Target tempo when `arbitrary` is enabled.  Values outside 10–480 BPM are clamped.  The legacy `arbval`/`interval_ms` fields are also honoured. |
| `skip` | bool | `false` | Emits every `(skip_val + 1)`-th analyzer beat and clears the beat flag otherwise. |
| `skip_val` | int | `1` | Number of beats to skip when `skip` is active (legacy `skipval`). |
| `skip_first` | int | `0` | Drops the first *n* analyzer beats before applying the selected mode (legacy `skipfirst`). |
| `invert` | bool | `false` | Swaps beat and non-beat frames. |
| `gate_enable` | bool | `true` | Controls whether the internal gate reacts to beat pulses.  When `false` the gate remains latched on. |
| `gate_hold` | int | `0` | Number of frames to keep the gate active after a beat when `sticky` is disabled. |
| `gate_sticky` | bool | `false` | Causes each beat pulse to toggle a sticky latch in the gate helper. |
| `gate_only_sticky` | bool | `false` | Requires the sticky latch to be engaged before reporting the gate as active. |
| `gate_register` | int | `-1` | Optional globals register (1–64) updated with the gate’s `render` state.  Negative values disable the bridge. |
| `gate_flag_register` | int | `-1` | Optional globals register (1–64) receiving the gate state (`0 = off`, `1 = beat`, `2 = hold`, `3 = sticky`). |

## Behaviour

* The effect samples the analyzer beat (`RenderContext::audioBeat` or
  `audioAnalysis->beat`) at the start of the frame.
* When `arbitrary` is enabled a periodic timer generates beat pulses using
  `RenderContext::deltaSeconds`.  Otherwise the chosen mode (`skip`, `invert`, or
  passthrough) modifies the analyzer beat on a frame-by-frame basis.
* Pulses are fed through `avs::effects::BeatGate` so the `gate_*` options mirror
  the gating controls used by transform effects.  The helper’s output drives the
  optional globals registers and informs the value written back to
  `RenderContext::audioBeat`.
* When no mode is selected the upstream beat passes through unchanged, but the
  gate still tracks the analyzer pulse train so consumers observing the globals
  bridge see consistent state.

The behaviour matches the Winamp plug-in defaults: the effect starts enabled in
arbitrary mode at 120 BPM with beat-skipping disabled.
