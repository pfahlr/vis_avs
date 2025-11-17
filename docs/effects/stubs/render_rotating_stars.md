# Render / Rotating Stars

This effect is now implemented in [`src/effects/render/effect_rotating_stars.cpp`](../../src/effects/render/effect_rotating_stars.cpp).
It draws two five-pointed stars that orbit around the screen centre. The star size reacts to
low-frequency peaks in the audio spectrum and the colour smoothly cycles through the configured
palette. Colours can be supplied as a list of hex values via the `colors` parameter, falling back to
white when unspecified.
