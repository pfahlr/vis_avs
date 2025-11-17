# Trans / Brightness

The modern renderer implements the legacy brightness curve using a per-channel lookup table that
matches `r_bright.cpp` from the original AVS source. Positive slider values brighten far more
aggressively than negative ones dim, mirroring the asymmetric UI response. The effect supports the
original additive (`Blend`) and average (`Blend Avg`) compositing modes, as well as the optional
color-exclusion mask driven by the reference color and distance threshold.
