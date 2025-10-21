# Trans / Color Clip

The Color Clip transform clamps darker pixels to a user-provided color. Pixels whose red, green, and
blue components are each less than or equal to the configured thresholds are replaced with the
configured color while preserving alpha. This mirrors the legacy Winamp AVS module
`R_ContrastEnhance` integer logic.
