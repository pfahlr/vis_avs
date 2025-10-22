# Render / Oscilloscope Star

This effect is now implemented in [`src/effects/render/effect_oscilloscope_star.cpp`](../../src/effects/render/effect_oscilloscope_star.cpp).
It renders five radial oscilloscope traces that pivot around a configurable anchor point. The
waveform amplitude modulates the length and curvature of each arm, producing a starburst that reacts
to the incoming audio. Colour palettes can be specified via hex strings (for example `#FF0000` or
comma-separated lists), individual `colorN` integer parameters, or the legacy `num_colors` and
`effect` flags used by AVS presets.
