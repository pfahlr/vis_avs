# Render / SVP Loader

The implementation lives in [`src/effects/render/effect_svp_loader.cpp`](../../src/effects/render/effect_svp_loader.cpp).
It attempts to load legacy Sonique Visual Plugin (`.svp` / `.uvs`) modules on Windows builds
and falls back to a safe no-op on other platforms.
