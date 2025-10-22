# CMake Impact Assessment

## New library targets
- `avs-base`
- `avs-math` (placeholder)
- `avs-core`
- `avs-audio-io`
- `avs-audio-dsp`
- `avs-compat`
- `avs-dsl`
- `avs-render-gl`
- `avs-effects-legacy`
- `avs-effects-modern` (placeholder)
- `avs-preset`
- `avs-testing`

Each target will define an `avs::` namespace alias (e.g. `avs::core`).

## Root `libs/CMakeLists.txt`
- Replace legacy `add_subdirectory(avs ...)`/`avs-core`/`avs-platform` structure with per-package subdirectories matching the new layout.
- Continue to include `third_party/ns-eel`.

## Root `CMakeLists.txt`
- Ensure `add_subdirectory(libs/avs-*)` entries match new packages and dependency order.

## Applications / tools / tests
- Update `target_link_libraries` to use the new `avs::` targets instead of the old monolithic libraries (`avs-core`, `avs-effects-core`, etc.).
- Adjust include directories to rely on public headers only; remove redundant `${CMAKE_SOURCE_DIR}/src` includes where possible.

## Dependency wiring
- `avs-base`: standalone, exported as `avs::base`.
- `avs-math`: depends on `avs::base`.
- `avs-core`: depends on `avs::base` and `avs::math`.
- `avs-audio-io`: depends on `avs::base` and `avs::audio-dsp`.
- `avs-audio-dsp`: depends on `avs::base` and `avs::math`.
- `avs-compat`: depends on `avs::core`, `avs::audio-io`, and `avs::audio-dsp` (plus `ns-eel`).
- `avs-dsl`: depends on `avs::core`, `avs::compat`, and `ns-eel`.
- `avs-render-gl`: depends on `avs::core`.
- `avs-effects-legacy`: depends on `avs::core`, `avs::dsl`, `avs::compat`, `avs::render-gl`, and `avs::preset`.
- `avs-effects-modern`: depends on `avs::core`, `avs::audio-dsp`, `avs::render-gl`.
- `avs-preset`: depends on `avs::core` and `avs::dsl`.
- `avs-testing`: depends on `avs::core`, `avs::audio-io`, `avs::audio-dsp`, `avs::compat`, `avs::preset`, and `avs::render-gl`.

## Removal / migration
- Deprecate `avs-platform`, `avs/audio`, and `avs/offscreen` targets; their sources move into the packages above.
- `avs-core` header-only compatibility pack becomes part of `avs-compat`.
- Provide compatibility aliases for legacy targets (`avs-core-runtime`, `avs-effects-core`, `avs-runtime`, `avs-offscreen`, `avs-audio`, `avs-platform`) to ease downstream migration.

