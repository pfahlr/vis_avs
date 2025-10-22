# Render / Timescope

The Timescope effect renders a scrolling column of audio data where the horizontal axis
represents time and the vertical axis scans through the most recent waveform samples.
Each frame advances to the next column, tinting the pixels using the configured color
and blend mode. The implementation pulls data from the modern audio analyzer and
supports additive, averaging, and adjustable line blending similar to the legacy AVS
module.
