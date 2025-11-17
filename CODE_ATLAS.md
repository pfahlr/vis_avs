# CODE_ATLAS.md

## Project: vis_avs - Advanced Visualization Studio (Modern Port)

This repository hosts a cross-platform reimplementation of Winamp's Advanced Visualization Studio (AVS), porting the legacy Windows-only codebase to modern C++17 with support for Linux and other platforms. The project features real-time audio visualization with PortAudio, OpenGL rendering, a comprehensive effect library, and deterministic testing infrastructure for regression validation.

---

## Directory Structure

### `/libs` - Core Libraries

#### `/libs/avs-core` - Core Effect Pipeline System
**Purpose:** Modern C++17 effect pipeline architecture with registry pattern

**Include Files:**
- `include/avs/core/IEffect.hpp` - Abstract interface for all renderable effects in the pipeline
- `include/avs/core/Pipeline.hpp` - Ordered collection of effects executed per frame
- `include/avs/core/EffectRegistry.hpp` - Registry mapping effect keys to factory functions
- `include/avs/core/RenderContext.hpp` - Mutable rendering context passed between effects
- `include/avs/core/ParamBlock.hpp` - Type-erased parameter block for effect configuration
- `include/avs/core/DeterministicRng.hpp` - Seeded random number generator for reproducible rendering

**Source Files:**
- `src/DeterministicRng.cpp` - Implementation of deterministic RNG for golden frame tests
- `src/EffectRegistry.cpp` - Effect factory registration and instantiation logic
- `src/Pipeline.cpp` - Effect chain execution and management

**Build Files:**
- `CMakeLists.txt` - Build configuration for avs-core library

---

#### `/libs/avs-compat` - Legacy Compatibility Layer
**Purpose:** Bridge between original AVS API and modern architecture

**Public Headers (avs/ namespace):**
- `include/avs/core.hpp` - Legacy compatibility wrapper for core types
- `include/avs/engine.hpp` - Legacy Engine class for frame-by-frame rendering
- `include/avs/effect.hpp` - Legacy Effect base class
- `include/avs/effects.hpp` - Legacy effect type enumerations
- `include/avs/effects_render.hpp` - Legacy render effect declarations
- `include/avs/effects_trans.hpp` - Legacy transform effect declarations
- `include/avs/effects_misc.hpp` - Legacy miscellaneous effect declarations
- `include/avs/preset.hpp` - Legacy preset loading and serialization
- `include/avs/registry.hpp` - Legacy effect registry
- `include/avs/params.hpp` - Legacy parameter handling
- `include/avs/eel.hpp` - EEL (Expression Evaluation Library) scripting integration
- `include/avs/cpu_features.hpp` - CPU feature detection (SSE, AVX)

**Modern Headers (avs/compat/ namespace):**
- `include/avs/compat/core.hpp` - Modern compatibility core types
- `include/avs/compat/engine.hpp` - Modern Engine wrapper
- `include/avs/compat/effect.hpp` - Modern effect compatibility layer
- `include/avs/compat/effects.hpp` - Modern effect type system
- `include/avs/compat/effects_render.hpp` - Modern render effects
- `include/avs/compat/effects_trans.hpp` - Modern transform effects
- `include/avs/compat/effects_misc.hpp` - Modern miscellaneous effects
- `include/avs/compat/effects_common.hpp` - Common effect utilities
- `include/avs/compat/preset.hpp` - Modern preset handling
- `include/avs/compat/registry.hpp` - Modern effect registry
- `include/avs/compat/params.hpp` - Modern parameter system
- `include/avs/compat/eel.hpp` - EEL scripting modern interface
- `include/avs/compat/cpu_features.hpp` - Modern CPU feature detection

**Runtime System:**
- `include/avs/runtime/GlobalState.hpp` - Global state management for legacy effects
- `include/avs/runtime/ResourceManager.hpp` - Resource lifecycle management
- `include/avs/runtime/framebuffers.h` - Framebuffer management interface
- `include/avs/compat/framebuffers_bridge.h` - Bridge between C and C++ framebuffer APIs
- `include/runtime/framebuffers.h` - Alternative framebuffer header path

**Source Files:**
- `src/core.cpp` - Core compatibility layer implementation
- `src/engine.cpp` - Legacy Engine implementation with framebuffer management
- `src/effect_registry.cpp` - Effect registration and factory system
- `src/preset.cpp` - Preset loading, parsing, and serialization
- `src/registry.cpp` - Global effect registry implementation
- `src/eel.cpp` - EEL scripting runtime integration
- `src/cpu_features.cpp` - Runtime CPU feature detection
- `src/effects_common.cpp` - Common effect utilities implementation
- `src/effects_render.cpp` - Render effect implementations
- `src/effects_render_geometry.cpp` - Geometric render effects
- `src/effects_trans.cpp` - Transform effect implementations
- `src/effects_misc.cpp` - Miscellaneous effect implementations
- `src/headless_main.cpp` - Headless rendering entry point for testing

**Runtime Implementation:**
- `src/runtime/ResourceManager.cpp` - Resource lifecycle and cleanup
- `src/runtime/framebuffers.cpp` - Framebuffer allocation and management
- `src/runtime/framebuffers_bridge.cpp` - C/C++ framebuffer API bridge

**Effect Implementations:**
- `src/effects/additive_blend.cpp` - Additive blending effect
- `src/effects/blur.cpp` - Box blur implementation
- `src/effects/color_transform.cpp` - Color space transformations
- `src/effects/colormap.cpp` - Color mapping and palette effects
- `src/effects/composite.cpp` - Layer compositing operations
- `src/effects/convolution.cpp` - Convolution filter implementation
- `src/effects/glow.cpp` - Glow/bloom effect
- `src/effects/mirror.cpp` - Mirror/reflection effect
- `src/effects/motion_blur.cpp` - Motion blur trail effect
- `src/effects/radial_blur.cpp` - Radial blur from center point
- `src/effects/scripted.cpp` - EEL-scripted custom effects
- `src/effects/tunnel.cpp` - Tunnel warping effect
- `src/effects/zoom_rotate.cpp` - Zoom and rotation transformations

**Geometry Effects:**
- `src/effects/geometry/raster.cpp` - Rasterization utilities
- `src/effects/geometry/raster.hpp` - Rasterization header
- `src/effects/geometry/superscope.cpp` - Superscope (scripted scope) effect
- `src/effects/geometry/superscope.hpp` - Superscope header
- `src/effects/geometry/text_renderer.cpp` - Text rendering implementation
- `src/effects/geometry/text_renderer.hpp` - Text rendering header

**Build Files:**
- `CMakeLists.txt` - Build configuration for avs-compat library

---

#### `/libs/avs-effects-legacy` - Legacy AVS Effect Implementations
**Purpose:** Port of original AVS effects with binary format compatibility

**Core Effects:**
- `include/avs/effects/core/effect.h` - Base effect class for legacy effects
- `include/avs/effects/core/effect_scripted.h` - Scripted effect base class
- `include/avs/effects/core/gating.h` - Audio gating/triggering system
- `include/avs/effects/core/Gating.hpp` - Modern gating interface
- `include/avs/effects/core/transform_affine.h` - Affine transformation effect
- `include/avs/effects/core/blend_ops.hpp` - Blend mode operations
- `src/core/effect.cpp` - Base effect implementation
- `src/core/effect_scripted.cpp` - Scripted effect runtime
- `src/core/gating.cpp` - Gating system implementation
- `src/core/transform_affine.cpp` - Affine transform implementation
- `src/core/blend_ops.cpp` - Blend operation implementations

**Dynamic/Movement Effects:**
- `include/avs/effects/dynamic/dynamic_shader.h` - Dynamic shader effect header
- `include/avs/effects/dynamic/movement.h` - Movement effect header
- `include/avs/effects/dynamic/dyn_movement.h` - Dynamic movement header
- `include/avs/effects/dynamic/dyn_distance.h` - Distance field movement header
- `include/avs/effects/dynamic/dyn_shift.h` - Dynamic shift effect header
- `include/avs/effects/dynamic/frame_warp.h` - Frame warping header
- `include/avs/effects/dynamic/zoom_rotate.h` - Zoom/rotate header
- `src/dynamic/dynamic_shader.cpp` - Programmable shader effects
- `src/dynamic/movement.cpp` - Pixel movement effects
- `src/dynamic/dyn_movement.cpp` - Dynamic movement implementation
- `src/dynamic/dyn_distance.cpp` - Distance field movements
- `src/dynamic/dyn_shift.cpp` - Dynamic shift implementation
- `src/dynamic/frame_warp.cpp` - Frame warping effects
- `src/dynamic/zoom_rotate.cpp` - Zoom and rotation effects

**Filter Effects:**
- `include/avs/effects/filters/effect_blur_box.h` - Box blur filter header
- `include/avs/effects/filters/effect_color_map.h` - Color map filter header
- `include/avs/effects/filters/effect_conv3x3.h` - 3x3 convolution filter header
- `include/avs/effects/filters/effect_fast_brightness.h` - Fast brightness adjustment header
- `include/avs/effects/filters/effect_grain.h` - Film grain effect header
- `include/avs/effects/filters/effect_interferences.h` - Interference patterns header
- `include/avs/effects/filters/filter_common.h` - Common filter utilities
- `src/filters/effect_blur_box.cpp` - Box blur implementation
- `src/filters/effect_color_map.cpp` - Color mapping implementation
- `src/filters/effect_conv3x3.cpp` - 3x3 convolution kernel
- `src/filters/effect_fast_brightness.cpp` - Fast brightness adjustment
- `src/filters/effect_grain.cpp` - Film grain generation
- `src/filters/effect_interferences.cpp` - Interference pattern generation

**Render Effects:**
- `include/avs/effects/render/effect_bass_spin.h` - Bass-driven spinning effect header
- `include/avs/effects/render/effect_dot_fountain.h` - Dot fountain particle effect header
- `include/avs/effects/render/effect_dot_plane.h` - Dot plane effect header
- `include/avs/effects/render/effect_moving_particle.h` - Moving particle system header
- `include/avs/effects/render/effect_oscilloscope_star.h` - Star-shaped oscilloscope header
- `include/avs/effects/render/effect_ring.h` - Ring oscilloscope header
- `include/avs/effects/render/effect_rotating_stars.h` - Rotating stars effect header
- `include/avs/effects/render/effect_simple_spectrum.h` - Simple spectrum analyzer header
- `include/avs/effects/render/effect_timescope.h` - Time-domain oscilloscope header
- `src/render/effect_bass_spin.cpp` - Bass spin implementation
- `src/render/effect_dot_fountain.cpp` - Dot fountain particles
- `src/render/effect_dot_plane.cpp` - Dot plane rendering
- `src/render/effect_moving_particle.cpp` - Particle system
- `src/render/effect_oscilloscope_star.cpp` - Star oscilloscope
- `src/render/effect_ring.cpp` - Ring oscilloscope
- `src/render/effect_rotating_stars.cpp` - Rotating stars
- `src/render/effect_simple_spectrum.cpp` - Simple spectrum analyzer
- `src/render/effect_svp_loader.cpp` - SVP (Sonic Visualiser Plugin) loader
- `src/render/effect_timescope.cpp` - Time-domain oscilloscope

**Transform Effects:**
- `include/avs/effects/trans/effect_blur.h` - Blur transform header
- `include/avs/effects/trans/effect_brightness.h` - Brightness adjustment header
- `include/avs/effects/trans/effect_channel_shift.h` - RGB channel shift header
- `include/avs/effects/trans/effect_color_clip.h` - Color clipping header
- `include/avs/effects/trans/effect_color_modifier.h` - Color modification header
- `include/avs/effects/trans/effect_colorfade.h` - Color fade effect header
- `include/avs/effects/trans/effect_fadeout.h` - Fadeout effect header
- `include/avs/effects/trans/effect_mosaic.h` - Mosaic/pixelation header
- `include/avs/effects/trans/effect_multi_delay.h` - Multi-delay feedback header
- `include/avs/effects/trans/effect_multiplier.h` - Multiplier effect header
- `include/avs/effects/trans/effect_roto_blitter.h` - Roto-blitter header
- `include/avs/effects/trans/effect_scatter.h` - Scatter/displacement header
- `include/avs/effects/trans/effect_unique_tone.h` - Unique tone filter header
- `include/avs/effects/trans/effect_video_delay.h` - Video delay buffer header
- `include/avs/effects/trans/effect_water.h` - Water ripple effect header
- `include/avs/effects/trans/effect_water_bump.h` - Water bump mapping header
- `include/avs/effects/trans/effect_transition.h` - Transition effect header
- `src/trans/effect_blur.cpp` - Blur transform implementation
- `src/trans/effect_brightness.cpp` - Brightness adjustment
- `src/trans/effect_channel_shift.cpp` - RGB channel shifting
- `src/trans/effect_color_clip.cpp` - Color clipping
- `src/trans/effect_color_modifier.cpp` - Color modification
- `src/trans/effect_color_modifier_registry.cpp` - Color modifier registry
- `src/trans/effect_color_reduction.cpp` - Color depth reduction
- `src/trans/effect_colorfade.cpp` - Color fade implementation
- `src/trans/effect_fadeout.cpp` - Fadeout implementation
- `src/trans/effect_mosaic.cpp` - Mosaic pixelation
- `src/trans/effect_multi_delay.cpp` - Multi-delay feedback
- `src/trans/effect_multiplier.cpp` - Multiplier implementation
- `src/trans/effect_roto_blitter.cpp` - Rotating blitter
- `src/trans/effect_scatter.cpp` - Scatter displacement
- `src/trans/effect_transition.cpp` - Effect transitions
- `src/trans/effect_unique_tone.cpp` - Unique tone filtering
- `src/trans/effect_video_delay.cpp` - Video delay buffer
- `src/trans/effect_water.cpp` - Water ripple effect
- `src/trans/effect_water_bump.cpp` - Water bump mapping
- `src/trans/effect_blitter_feedback.cpp` - Blitter feedback effect

**Miscellaneous Effects:**
- `include/avs/effects/misc/effect_comment.h` - Comment/annotation effect header
- `include/avs/effects/misc/effect_custom_bpm.h` - Custom BPM detector header
- `include/avs/effects/misc/effect_render_mode.h` - Render mode control header
- `include/avs/effects/misc/effect_unknown_render_object.h` - Unknown effect placeholder header
- `src/misc/effect_comment.cpp` - Comment effect (no-op with metadata)
- `src/misc/effect_custom_bpm.cpp` - Custom BPM detection
- `src/misc/effect_render_mode.cpp` - Render mode switching
- `src/misc/effect_unknown_render_object.cpp` - Unknown effect handler

**Prime Effects (Modern Rewrites):**
- `include/avs/effects/prime/AudioOverlays.hpp` - Audio visualization overlays header
- `include/avs/effects/prime/Blend.hpp` - Modern blend effect header
- `include/avs/effects/prime/Bump.hpp` - Bump mapping effect header
- `include/avs/effects/prime/Clear.hpp` - Clear/fill effect header
- `include/avs/effects/prime/Globals.hpp` - Global effect state header
- `include/avs/effects/prime/Overlay.hpp` - Layer overlay effect header
- `include/avs/effects/prime/Swizzle.hpp` - Channel swizzling header
- `include/avs/effects/prime/TransformAffine.hpp` - Affine transformation header
- `include/avs/effects/prime/Zoom.hpp` - Zoom effect header
- `include/avs/effects/prime/RegisterEffects.hpp` - Effect registration utilities
- `include/avs/effects/prime/micro_preset_parser.hpp` - Micro-preset parser header
- `src/prime/AudioOverlays.cpp` - Audio overlay implementation
- `src/prime/Blend.cpp` - Blend effect implementation
- `src/prime/Bump.cpp` - Bump mapping implementation
- `src/prime/Clear.cpp` - Clear/fill implementation
- `src/prime/Globals.cpp` - Global state implementation
- `src/prime/Overlay.cpp` - Layer overlay implementation
- `src/prime/Swizzle.cpp` - Channel swizzle implementation
- `src/prime/Text.cpp` - Text rendering implementation
- `src/prime/TransformAffine.cpp` - Affine transform implementation
- `src/prime/Zoom.cpp` - Zoom effect implementation
- `src/prime/RegisterEffects.cpp` - Effect registration
- `src/prime/micro_preset_parser.cpp` - Micro-preset parser

**Primitives (Shapes):**
- `include/avs/effects/primitives/Primitives.hpp` - Primitive shape effect header
- `include/avs/effects/primitives/primitive_common.hpp` - Common primitive utilities
- `src/primitives/primitive_dots.cpp` - Dot primitive rendering
- `src/primitives/primitive_lines.cpp` - Line primitive rendering
- `src/primitives/primitive_rrect.cpp` - Rounded rectangle primitives
- `src/primitives/primitive_solid.cpp` - Solid fill primitives
- `src/primitives/primitive_tri.cpp` - Triangle primitives

**Text Rendering:**
- `include/avs/effects/text/text_renderer.h` - Text rendering interface header
- `src/text/text_renderer.cpp` - Text rendering implementation

**Stub Effects:**
- `src/stubs/effect_render_avi.cpp` - AVI video rendering stub (not implemented)

**Registry & Graph:**
- `include/avs/effects/registry.h` - Effect registry interface
- `include/avs/effects/effect_registry.hpp` - Modern effect registry
- `include/avs/effects/graph.h` - Effect graph/pipeline header
- `include/avs/effects/api.h` - Public API declarations
- `include/avs/effects/legacy_effect.h` - Legacy effect wrapper
- `include/avs/effects/legacy/register_all.h` - Bulk effect registration header
- `src/registry.cpp` - Registry implementation
- `src/effect_registry.cpp` - Modern registry implementation
- `src/graph.cpp` - Effect graph implementation
- `src/register_all.cpp` - Bulk effect registration

**Build Files:**
- `CMakeLists.txt` - Build configuration for avs-effects-legacy library

---

#### `/libs/avs-audio-io` - Audio Input/Output System
**Purpose:** PortAudio-based audio capture with device negotiation

**Headers:**
- `include/avs/audio.hpp` - Legacy audio interface wrapper
- `include/avs/audio/audio.hpp` - Modern audio interface
- `include/avs/audio/AudioEngine.hpp` - Audio engine with device management
- `include/avs/audio/DeviceInfo.hpp` - Audio device information structures
- `include/avs/audio/audio_portaudio_internal.hpp` - PortAudio implementation details
- `include/avs/audio_portaudio_internal.hpp` - Legacy PortAudio wrapper

**Source Files:**
- `src/AudioEngine.cpp` - Audio engine implementation with PortAudio
- `src/DeviceInfo.cpp` - Device enumeration and selection logic
- `src/audio_portaudio.cpp` - PortAudio callback and stream management

**Build Files:**
- `CMakeLists.txt` - Build configuration for avs-audio-io library

---

#### `/libs/avs-audio-dsp` - Audio DSP & Analysis
**Purpose:** FFT-based audio analysis using Kiss FFT

**Headers:**
- `include/avs/fft.hpp` - Legacy FFT interface wrapper
- `include/avs/audio/fft.hpp` - Modern FFT interface
- `include/avs/audio/analyzer.h` - Audio spectrum analyzer

**Source Files:**
- `src/fft_kiss.cpp` - Kiss FFT wrapper implementation
- `src/analyzer.cpp` - Spectrum analyzer with beat detection

**Build Files:**
- `CMakeLists.txt` - Build configuration for avs-audio-dsp library

---

#### `/libs/avs-render-gl` - OpenGL Rendering Backend
**Purpose:** SDL2 + OpenGL window management

**Headers:**
- `include/avs/window.hpp` - Legacy window interface
- `include/avs/render/gl/window.hpp` - Modern OpenGL window interface

**Source Files:**
- `src/window_sdl_gl.cpp` - SDL2 + OpenGL window implementation with vsync control

**Build Files:**
- `CMakeLists.txt` - Build configuration for avs-render-gl library

---

#### `/libs/avs-preset` - Preset Parser & Binder
**Purpose:** Binary AVS preset parsing and effect instantiation

**Headers:**
- `include/avs/preset/parser.h` - Legacy preset text parser
- `include/avs/preset/ir.h` - Intermediate representation for presets
- `include/avs/preset/binder.h` - Preset-to-effect binding system

**Source Files:**
- `src/parser.cpp` - Text-based preset parser implementation
- `src/binder.cpp` - Binary preset parser and effect binder

**Tests:**
- `tests/test_ir_and_binder.cpp` - Unit tests for IR and binder

**Build Files:**
- `CMakeLists.txt` - Build configuration for avs-preset library

---

#### `/libs/avs-base` - Platform Abstraction Layer
**Purpose:** Cross-platform filesystem and platform utilities

**Headers:**
- `include/avs/platform.hpp` - Legacy platform utilities
- `include/avs/fs.hpp` - Legacy filesystem utilities
- `include/avs/base/platform.hpp` - Modern platform abstraction
- `include/avs/base/fs.hpp` - Modern filesystem utilities

**Source Files:**
- `src/platform.cpp` - Platform detection and utilities
- `src/fs_posix.cpp` - POSIX filesystem implementation

**Build Files:**
- `CMakeLists.txt` - Build configuration for avs-base library

---

#### `/libs/avs-testing` - Test Utilities
**Purpose:** Offscreen rendering and MD5 hashing for golden frame tests

**Headers:**
- `include/avs/testing/OffscreenRenderer.hpp` - Offscreen rendering interface
- `include/avs/testing/Md5.hpp` - MD5 hashing for frame comparison
- `include/avs/offscreen/OffscreenRenderer.hpp` - Alternative offscreen renderer path
- `include/avs/offscreen/Md5.hpp` - Alternative MD5 path

**Source Files:**
- `src/OffscreenRenderer.cpp` - Headless OpenGL rendering for tests
- `src/Md5.cpp` - MD5 checksum computation

**Build Files:**
- `CMakeLists.txt` - Build configuration for avs-testing library

---

#### `/libs/avs-dsl` - Domain-Specific Language Runtime
**Purpose:** EEL (Expression Evaluation Language) scripting runtime

**Headers:**
- `include/avs/runtime/script/eel_runtime.h` - EEL runtime interface
- `src/script/eel_runtime.h` - EEL runtime internal header

**Source Files:**
- `src/script/eel_runtime.cpp` - EEL scripting engine integration with ns-eel

**Build Files:**
- `CMakeLists.txt` - Build configuration for avs-dsl library

---

#### `/libs/avs-math` - Math Utilities
**Purpose:** Vector math and SIMD utilities (currently empty, placeholder for future work)

**Build Files:**
- `CMakeLists.txt` - Build configuration for avs-math library

---

#### `/libs/avs-effects-modern` - Modern Effect Implementations
**Purpose:** Clean-room rewrites of effects with modern C++ (currently empty, placeholder)

**Build Files:**
- `CMakeLists.txt` - Build configuration for avs-effects-modern library

---

#### `/libs/avs-platform` - Platform-Specific Code
**Purpose:** Platform-specific implementations (currently empty, placeholder)

**Build Files:**
- `CMakeLists.txt` - Build configuration for avs-platform library

---

#### `/libs/third_party` - Vendored Dependencies
**Purpose:** Third-party libraries integrated into the build

**ns-eel (Nullsoft Expression Evaluation Library):**
- `ns-eel/ns-eel.h` - Main EEL API header
- `ns-eel/ns-eel-int.h` - Internal EEL definitions
- `ns-eel/ns-eel-addfuncs.h` - Additional EEL function macros
- `ns-eel/nseel-compiler.c` - EEL expression compiler
- `ns-eel/nseel-eval.c` - EEL expression evaluator
- `ns-eel/nseel-cfunc.c` - Built-in EEL functions
- `ns-eel/nseel-caltab.c` - EEL operator table
- `ns-eel/nseel-lextab.c` - EEL lexer table
- `ns-eel/nseel-ram.c` - EEL memory management
- `ns-eel/nseel_lexer_decls.h` - Lexer declarations
- `ns-eel/y.tab.c` - Generated parser (yacc/bison)
- `ns-eel/y.tab.h` - Parser header
- `ns-eel/wdltypes.h` - WDL (Cockos) type definitions
- `ns-eel/wdlcstring.h` - WDL C string utilities
- `ns-eel/utf8_extended.h` - UTF-8 string helpers
- `ns-eel/glue_port.h` - Platform glue code
- `ns-eel/denormal.h` - Denormal float handling
- `ns-eel/CMakeLists.txt` - ns-eel build configuration

**Kiss FFT:**
- `kissfft/kiss_fft.h` - Kiss FFT main header
- `kissfft/kiss_fft.c` - Kiss FFT implementation
- `kissfft/kiss_fftr.h` - Real FFT wrapper
- `kissfft/kiss_fftr.c` - Real FFT implementation
- `kissfft/_kiss_fft_guts.h` - Internal Kiss FFT definitions
- `kissfft/kiss_fft_log.h` - Kiss FFT logging utilities

**Other Libraries:**
- `sha256.h` - SHA-256 hashing interface
- `sha256.c` - SHA-256 implementation
- `dr_wav.h` - WAV file reader (header-only)
- `stb_image.h` - Image loading library (header-only)
- `stb_image_write.h` - Image writing library (header-only)

---

#### `/libs/attic` - Deprecated Code
**Purpose:** Archived code not currently in use

**Documentation:**
- `README.md` - Explanation of archived code

---

### `/apps` - Applications

#### `/apps/avs-player` - Standalone Player
**Source Files:**
- `main.cpp` - Entry point for standalone AVS player with audio capture and preset loading

**Build Files:**
- `CMakeLists.txt` - Build configuration for avs-player executable

---

#### `/apps/avs-studio` - Visual Editor (Work in Progress)
**Source Files:**
- `main.cpp` - Entry point for AVS visual editor/studio application (MVP in development)

**Build Files:**
- `CMakeLists.txt` - Build configuration for avs-studio executable

---

### `/tests` - Test Suite

**Top-Level Tests:**
- `avs_core_tests.cpp` - Core system integration tests
- `deterministic_render_test.cpp` - Deterministic rendering validation
- `audio_underflow_tests.cpp` - Audio buffer underflow handling tests

**Regression Tests:**
- `regression/test_offscreen_golden.cpp` - Golden frame MD5 snapshot comparison tests

**Core Effect Tests:**
- `core/test_pipeline.cpp` - Pipeline execution tests
- `core/test_effect_registry.cpp` - Effect registry tests
- `core/test_scripted_effect.cpp` - Scripted effect (EEL) tests
- `core/test_gating.cpp` - Audio gating tests
- `core/test_globals_and_bump.cpp` - Global state and bump mapping tests
- `core/test_blend_ops.cpp` - Blend operation tests
- `core/test_color_modifier.cpp` - Color modification tests
- `core/test_channel_shift.cpp` - RGB channel shift tests
- `core/test_colorfade.cpp` - Color fade tests
- `core/test_transform_affine.cpp` - Affine transformation tests
- `core/test_video_delay.cpp` - Video delay buffer tests
- `core/test_trans_brightness.cpp` - Brightness adjustment tests
- `core/test_trans_unique_tone.cpp` - Unique tone filter tests
- `core/test_trans_scatter.cpp` - Scatter effect tests
- `core/test_trans_mosaic.cpp` - Mosaic effect tests
- `core/test_trans_multiplier.cpp` - Multiplier effect tests
- `core/test_multi_delay.cpp` - Multi-delay effect tests
- `core/test_roto_blitter.cpp` - Roto-blitter tests
- `core/test_misc_comment.cpp` - Comment effect tests
- `core/test_misc_custom_bpm.cpp` - Custom BPM detector tests
- `core/test_render_mode.cpp` - Render mode switching tests
- `core/test_render_bass_spin.cpp` - Bass spin effect tests
- `core/test_render_dot_fountain.cpp` - Dot fountain tests
- `core/test_render_moving_particle.cpp` - Moving particle tests
- `core/test_render_oscilloscope_star.cpp` - Oscilloscope star tests
- `core/test_render_ring.cpp` - Ring oscilloscope tests
- `core/test_render_rotating_stars.cpp` - Rotating stars tests
- `core/test_render_timescope.cpp` - Timescope tests
- `core/test_simple_spectrum.cpp` - Simple spectrum analyzer tests
- `core/test_primitives.cpp` - Primitive shape tests

**Audio Tests:**
- `audio/test_analyzer.cpp` - Audio spectrum analyzer tests
- `audio/test_device_negotiation.cpp` - Audio device selection tests

**Runtime Tests:**
- `runtime/test_resources.cpp` - Resource management tests
- `runtime/test_runtime_compositing.cpp` - Compositing system tests
- `runtime/effect_list_parser_tests.cpp` - Effect list parser tests
- `runtime/parser/effect_list_parser_tests.cpp` - Alternative parser test path

**Preset Tests:**
- `presets/fb_ops/test_fb_ops.cpp` - Framebuffer operation preset tests
- `presets/fb_ops/md5_helper.cpp` - MD5 helper for preset tests
- `presets/fb_ops/md5_helper.hpp` - MD5 helper header
- `presets/audio_vis/test_audio_vis.cpp` - Audio visualization preset tests
- `presets/trans/test_trans.cpp` - Transform preset tests
- `presets/trans/test_trans_blur.cpp` - Blur transform preset tests
- `presets/trans/test_blitter_feedback.cpp` - Blitter feedback preset tests
- `presets/dynamic/test_dynamic.cpp` - Dynamic effect preset tests
- `presets/filters/test_filters.cpp` - Filter effect preset tests

**Build Files:**
- `CMakeLists.txt` - Test suite build configuration

---

### `/tools` - Build & Development Tools

**Tools:**
- `gen_golden_md5.cpp` - CLI tool to generate golden MD5 hashes for regression tests

**Build Files:**
- `CMakeLists.txt` - Tools build configuration

---

### `/codex` - Project Documentation & Jobs

**Top-Level Documentation:**
- `intake.md` - Codex intake and onboarding documentation
- `TODO.md` - Project-wide TODO tracking

**Job Specifications (YAML):**
- `jobs/00-bootstrap.yaml` - Bootstrap job for initial setup
- `jobs/01-warn-nseel-prototypes.yaml` - Fix ns-eel function prototype warnings
- `jobs/02-warn-sse-cast-align.yaml` - Fix SSE cast-align warnings
- `jobs/03-warn-portaudio-vendor.yaml` - PortAudio vendoring warnings
- `jobs/04-warn-gl-pointer-casts.yaml` - OpenGL pointer cast warnings
- `jobs/05-presets-gui-preset-cli.yaml` - Preset CLI tool job
- `jobs/06-presets-binary-parser.yaml` - Binary preset parser implementation
- `jobs/07-presets-extended-header-fix.yaml` - Extended preset header fixes
- `jobs/08-audio-resampling-negotiation.yaml` - Audio resampling and device negotiation
- `jobs/09-engine-pa-underflow-surfacing.yaml` - PortAudio underflow surfacing
- `jobs/10-engine-wav-in-gui.yaml` - WAV file input in GUI
- `jobs/11-engine-input-device-select.yaml` - Audio input device selection
- `jobs/12-engine-eel-scripted-parity.yaml` - EEL scripting parity with original
- `jobs/13-data-fix-manifest-duplicates.yaml` - Fix manifest duplicate entries
- `jobs/14-presets-complete-binary-parser.yaml` - Complete binary parser coverage
- `jobs/15-effects-implement-transition.yaml` - Implement transition effects
- `jobs/16-effects-implement-superscope.yaml` - Implement Superscope effect
- `jobs/17-effects-implement-core-missing.yaml` - Implement missing core effects
- `jobs/18-presets-json-format-support.yaml` - JSON preset format support
- `jobs/19-render-multithreaded-effects.yaml` - Multithreaded effect rendering
- `jobs/20-audio-pipewire-backend.yaml` - PipeWire audio backend
- `jobs/21-ui-studio-mvp.yaml` - Studio UI MVP implementation
- `jobs/22-render-framebuffer-abstraction.yaml` - Framebuffer abstraction layer
- `jobs/23-integration-demo-preset-validation.yaml` - Demo preset validation
- `jobs/24-cleanup-registry-consolidation.yaml` - Registry consolidation cleanup
- `jobs/README.md` - Job system documentation

**Visual Effects Specifications:**
- `specs/visual_effects/visual_effects_master.yaml` - Master visual effects specification
- `specs/visual_effects/components/base_visual_effects.yaml` - Base visual effects component spec
- `specs/visual_effects/components/core_visual_effects.yaml` - Core visual effects component spec

---

### `/docs` - Documentation

**Top-Level Documentation:**
- `README.md` - Main documentation entry point with build instructions
- `compat.md` - Compatibility notes with original AVS
- `vis_avs_effects.TODO.md` - Effect implementation TODO list
- `preset_loader_missing_effects.md` - Missing effect documentation for preset loader
- `preset_bind_registry.md` - Preset binding and registry documentation
- `ci_improvements.TODO.md` - CI/CD improvement TODO list

**Effect Documentation:**
- `effects/audio.md` - Audio visualization effects documentation
- `effects/blend.md` - Blend mode documentation
- `effects/bump.md` - Bump mapping effect documentation
- `effects/compositing.md` - Compositing system documentation
- `effects/dynamic.md` - Dynamic/movement effects documentation
- `effects/fb_ops.md` - Framebuffer operations documentation
- `effects/filters.md` - Filter effects documentation
- `effects/framebuffers.md` - Framebuffer system documentation
- `effects/gating.md` - Audio gating documentation
- `effects/globals.md` - Global effect state documentation
- `effects/primitives.md` - Primitive shapes documentation
- `effects/scripted.md` - Scripted effects (EEL) documentation
- `effects/shapes.md` - Shape rendering documentation
- `effects/superscope.md` - Superscope effect documentation
- `effects/text.md` - Text rendering documentation
- `effects/transforms.md` - Transform effects documentation
- `effects/trans_blur.md` - Blur transform documentation
- `effects/misc/custom_bpm.md` - Custom BPM detector documentation
- `effects/render/dot_fountain.md` - Dot fountain effect documentation
- `effects/trans/mosaic.md` - Mosaic effect documentation
- `effects/trans/multiplier.md` - Multiplier effect documentation
- `effects/trans/video_delay.md` - Video delay effect documentation

**Stub Effect Documentation:**
- `effects/stubs/channel_shift.md` - Channel shift stub documentation
- `effects/stubs/holden05_multi_delay.md` - Multi-delay stub documentation
- `effects/stubs/misc_comment.md` - Comment effect stub documentation
- `effects/stubs/misc_set_render_mode.md` - Render mode stub documentation
- `effects/stubs/render_avi.md` - AVI rendering stub documentation
- `effects/stubs/render_bass_spin.md` - Bass spin stub documentation
- `effects/stubs/render_dot_plane.md` - Dot plane stub documentation
- `effects/stubs/render_moving_particle.md` - Moving particle stub documentation
- `effects/stubs/render_oscilloscope_star.md` - Oscilloscope star stub documentation
- `effects/stubs/render_ring.md` - Ring stub documentation
- `effects/stubs/render_rotating_stars.md` - Rotating stars stub documentation
- `effects/stubs/render_simple.md` - Simple render stub documentation
- `effects/stubs/render_svp_loader.md` - SVP loader stub documentation
- `effects/stubs/render_timescope.md` - Timescope stub documentation
- `effects/stubs/trans_blitter_feedback.md` - Blitter feedback stub documentation
- `effects/stubs/trans_brightness.md` - Brightness stub documentation
- `effects/stubs/trans_color_clip.md` - Color clip stub documentation
- `effects/stubs/trans_color_modifier.md` - Color modifier stub documentation
- `effects/stubs/trans_colorfade.md` - Colorfade stub documentation
- `effects/stubs/trans_roto_blitter.md` - Roto-blitter stub documentation
- `effects/stubs/trans_scatter.md` - Scatter stub documentation
- `effects/stubs/trans_unique_tone.md` - Unique tone stub documentation
- `effects/stubs/trans_water.md` - Water effect stub documentation
- `effects/stubs/trans_water_bump.md` - Water bump stub documentation

**Legacy Implementation Documentation:**
- `legacy_effects/phase1.md` - Phase 1 legacy effect implementation notes

**Original Source Port Notes:**
- `avs_original_source_port_notes/ape-plugin-architecture.md` - APE plugin architecture notes
- `avs_original_source_port_notes/evallib-expression-evaluator.md` - Evallib expression evaluator notes
- `avs_original_source_port_notes/laser-effects-architecture.md` - Laser effects architecture notes
- `avs_original_source_port_notes/ns-eel-expression-language.md` - NS-EEL expression language notes

**Compatibility Manifest:**
- `compat/manifest.yaml` - Compatibility manifest for effect coverage
- `effects_manifest.yaml` - Effects manifest

---

### Root Configuration Files

**Build System:**
- `CMakeLists.txt` - Root CMake build configuration
- `libs/CMakeLists.txt` - Libraries CMake configuration
- `apps/CMakeLists.txt` - Applications CMake configuration
- `tests/CMakeLists.txt` - Tests CMake configuration
- `tools/CMakeLists.txt` - Tools CMake configuration

**Version Control:**
- `.gitignore` - Git ignore patterns for build artifacts and IDE files

**Code Formatting:**
- `.clang-format` - Clang-format style configuration (Google style, 100-col width)
- `.editorconfig` - Editor configuration for consistent code style

**Scripts:**
- `run_build.sh` - Automated build script for quick compilation
- `run_setup_dev_environment.sh` - Development environment setup script (installs dependencies)

**Documentation:**
- `README.md` - Project overview, build instructions, and usage examples
- `CONTRIBUTING.md` - Contribution guidelines
- `AGENTS.md` - Agent/Codex setup instructions and development workflow

**Patches:**
- `avs.1.patch` - Patch file for AVS compatibility fixes

**Tracking:**
- `unimlemented.lst` - List of unimplemented effects and features

---

## Key Architecture Patterns

### Effect System
The project uses a dual architecture:
1. **Modern Core** (`avs-core`): Clean C++17 with `IEffect` interface, `Pipeline` execution, and `EffectRegistry` pattern
2. **Legacy Compat** (`avs-compat`, `avs-effects-legacy`): Binary-compatible implementation of original AVS effects

### Audio Pipeline
- **Input**: PortAudio with automatic device negotiation
- **Analysis**: Kiss FFT-based spectrum analyzer with beat detection
- **DSP**: Modular audio processing in `avs-audio-dsp`

### Rendering
- **Primary**: OpenGL with SDL2 window management
- **Testing**: Offscreen headless renderer with deterministic seeding

### Preset System
- **Binary Parser**: Full compatibility with legacy `.avs` preset files
- **IR Layer**: Intermediate representation for preset transformations
- **Binder**: Maps preset effect blocks to runtime effect instances

### Testing Strategy
- **Unit Tests**: GoogleTest for individual components
- **Golden Frame Tests**: MD5 checksums of rendered frames for regression detection
- **Deterministic RNG**: Seeded random numbers for reproducible rendering across platforms

---

## Development Workflow

1. **Setup**: Run `./run_setup_dev_environment.sh` to install dependencies
2. **Build**: Use `cmake` to configure and `make` or `ninja` to build
3. **Test**: Run `ctest` from the build directory
4. **Format**: Apply `clang-format` before committing
5. **CI**: GitHub Actions validate builds, tests, and formatting

---

## Effect Coverage Status

The project has achieved **full coverage (56/56)** of original AVS effects:
- ✅ All core transform, render, and filter effects implemented
- ✅ APE (plugin) effects ported
- ✅ Laser effects implemented
- ✅ Binary preset parser supports all effect types

For detailed effect status, see `docs/effects_manifest.yaml` and `codex/specs/visual_effects/`.

---

*This CODE_ATLAS.md is auto-generated and should be updated when major structural changes occur.*
