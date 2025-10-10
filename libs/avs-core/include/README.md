# AVS Starter Headers (C++20)

This header-only starter pack mirrors the merged YAML spec for the Linux AVS pipeline.
It defines the core types, parameter schema, abstract effect interfaces, and declarations
for the base effects (Render / Trans / Misc). Implementations should live in your `.cpp` files.

## Suggested Libraries (drop-in / optional)
- **Audio**: PortAudio (Pulse/JACK/WASAPI), or JACK directly.
- **FFT**: kissFFT or FFTW3.
- **Window/IO**: SDL2 or SDL3 (streaming RGBA texture upload).
- **Fonts**: FreeType or SDL_ttf.
- **Images**: stb_image (PNG/JPG).
- **Scripting**: Cockos WDL **EEL2** (NSEEL-compatible surface).

## Files
- `avs/core.hpp` – common types, audio features, frame buffers, enums.
- `avs/params.hpp` – parameter system and schema structures.
- `avs/effect.hpp` – base `IEffect` interface and contexts.
- `avs/registry.hpp` – effect registry + factory.
- `avs/effects_render.hpp` – render effect declarations.
- `avs/effects_trans.hpp` – transform effect declarations.
- `avs/effects_misc.hpp` – misc/control effect declarations.

## Notes
- Indentation uses **2 spaces** as requested.
- This is a **starter** API surface; fill in implementations, sampling, and EEL bindings in `.cpp`.
- Blend/sampling modes match the YAML and classic AVS semantics.
