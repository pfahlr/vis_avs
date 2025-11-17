# vis_avs Codebase Architecture

**Last Updated**: 2025-11-17
**Version**: 0.1.0

## Table of Contents

1. [Overview](#overview)
2. [Directory Structure](#directory-structure)
3. [Core Libraries](#core-libraries)
4. [Applications](#applications)
5. [Build System](#build-system)
6. [Preset System](#preset-system)
7. [Effect System](#effect-system)
8. [Audio Pipeline](#audio-pipeline)
9. [Rendering Pipeline](#rendering-pipeline)
10. [Testing](#testing)
11. [Development Workflow](#development-workflow)

---

## Overview

vis_avs is a modern C++20 reimplementation of the classic Winamp AVS (Advanced Visualization Studio) plugin. The project aims to provide:

- **Binary compatibility** with original .avs presets
- **Modern architecture** with clean separation of concerns
- **Cross-platform support** (Linux primary, Windows/macOS future)
- **Performance improvements** including multi-threading
- **Developer-friendly** preset format (JSON)

### Design Principles

1. **Modularity**: Clear library boundaries with minimal dependencies
2. **Compatibility**: Support original AVS presets and effects
3. **Performance**: Optimize hot paths, support multi-threading
4. **Extensibility**: Plugin-style effect registration
5. **Testability**: Unit tests, golden hash validation

---

## Directory Structure

```
vis_avs/
├── apps/                    # End-user applications
│   ├── avs-player/         # Command-line preset player
│   └── avs-studio/         # GUI preset editor (stub)
│
├── libs/                    # Core libraries (modular architecture)
│   ├── avs-base/           # Platform abstractions (filesystem, etc.)
│   ├── avs-core/           # Effect interface, pipeline, registry
│   ├── avs-preset/         # Preset parsing (text/binary/JSON)
│   ├── avs-dsl/            # EEL scripting runtime
│   ├── avs-audio-dsp/      # FFT, spectrum analysis
│   ├── avs-audio-io/       # Audio capture (PortAudio)
│   ├── avs-audio-pipewire/ # Pipewire backend (Linux, WIP)
│   ├── avs-compat/         # Legacy AVS compatibility layer
│   ├── avs-effects-legacy/ # Port of original AVS effects
│   ├── avs-math/           # Vector math, SIMD utilities
│   └── third_party/        # External dependencies
│
├── tools/                   # Development utilities
│   ├── avs-convert         # Preset format converter
│   ├── gen_golden_md5      # Golden hash generator
│   └── audit_effects.py    # Codebase analysis scripts
│
├── tests/                   # Test suite
│   ├── unit tests          # Library-level tests
│   ├── integration tests   # End-to-end rendering tests
│   └── data/               # Test presets, golden hashes
│
├── docs/                    # Documentation
│   ├── CODEBASE_ARCHITECTURE.md (this file)
│   ├── preset_json_schema.md
│   ├── studio_ui_architecture.md
│   └── avs_original_source_port_notes/
│
├── codex/                   # Development roadmap
│   └── jobs/               # YAML job definitions (tasks 00-24)
│
└── resources/              # Runtime resources (fonts, shaders)
```

---

## Core Libraries

### avs-base

**Purpose**: Platform abstractions and utilities
**Dependencies**: None
**Key Files**:
- `include/avs/base/fs.hpp` - Filesystem operations
- `include/avs/base/platform.hpp` - Platform detection

### avs-core

**Purpose**: Core rendering infrastructure
**Dependencies**: avs-base, avs-math
**Key Concepts**:

```cpp
// Effect interface - all effects implement this
class IEffect {
  virtual bool render(RenderContext& context) = 0;
  virtual bool smp_render(RenderContext& ctx, int tid, int threads);
  virtual void setParams(const ParamBlock& params) = 0;
};

// Rendering context - passed to each effect
struct RenderContext {
  std::uint64_t frameIndex;
  int width, height;
  PixelBufferView framebuffer;
  AudioBufferView audioSpectrum;
  bool audioBeat;
  DeterministicRng rng;
};

// Pipeline - chains effects together
class Pipeline {
  void addEffect(std::unique_ptr<IEffect> effect);
  bool render(RenderContext& context);
};
```

**Key Files**:
- `include/avs/core/IEffect.hpp` - Effect interface
- `include/avs/core/RenderContext.hpp` - Per-frame state
- `include/avs/core/Pipeline.hpp` - Effect chain execution
- `include/avs/core/ThreadPool.hpp` - Multi-threading support
- `src/EffectRegistry.cpp` - Effect factory registration

### avs-preset

**Purpose**: Preset parsing and serialization
**Dependencies**: avs-core, avs-dsl, avs-effects-api
**Formats Supported**:
- Text format (legacy, line-based)
- Binary .avs (Winamp AVS format)
- JSON (modern, human-readable)

**Key Types**:

```cpp
// Intermediate representation (IR)
struct IRParam {
  std::string name;
  enum Kind { F32, I32, BOOL, STR } kind;
  float f; int i; bool b; std::string s;
};

struct IRNode {
  std::string token;          // Effect type (e.g., "Render / Simple")
  std::vector<IRParam> params;
  std::vector<IRNode> children;  // Nested effects (for containers)
  int order_index;
};

struct IRPreset {
  std::vector<IRNode> root_nodes;
  std::string compat;  // "strict" or "best-effort"
};
```

**Key Functions**:
- `parse_legacy_preset(text)` - Parse text/binary preset → IRPreset
- `serializeToJson(preset)` - IRPreset → JSON string
- `deserializeFromJson(json)` - JSON string → IRPreset

### avs-dsl

**Purpose**: EEL (Electro Expression Language) scripting
**Dependencies**: ns-eel (third-party), avs-core
**Usage**: Scripted effects (SuperScope, DynamicMovement, etc.)

**Example**:
```cpp
// EEL script in preset
"init": "n=100; t=0",
"frame": "t=t+0.01",
"point": "x=cos(i*$PI*2+t); y=sin(i*$PI*2+t)"
```

### avs-audio-dsp

**Purpose**: Audio analysis and FFT
**Dependencies**: kiss_fft (third-party), avs-base
**Features**:
- Real-time FFT (spectrum analysis)
- Beat detection
- RMS/peak metering
- Octave bands

**Key Classes**:
- `FFT` - Fast Fourier Transform wrapper
- `Analyzer` - Audio feature extraction

### avs-audio-io

**Purpose**: Audio capture abstraction
**Dependencies**: PortAudio, avs-audio-dsp
**Backends**:
- PortAudio (default, cross-platform)
- Pipewire (Linux, WIP in avs-audio-pipewire)

### avs-compat

**Purpose**: Legacy AVS compatibility layer
**Dependencies**: avs-core, avs-dsl, avs-effects-legacy
**Functionality**:
- Binary .avs preset parser (original format)
- Legacy effect instantiation
- Compatibility shims for older preset features

### avs-effects-legacy

**Purpose**: Ports of original AVS effects
**Dependencies**: avs-core, avs-dsl
**Effect Count**: 56 effect types implemented

**Categories**:
```
Render/  - Visual generators (Simple, Superscope, Particles, etc.)
Trans/   - Transformations (Blur, Water, Movement, etc.)
Misc/    - Utilities (Buffer Save, Comment, BPM, etc.)
```

**Registration Pattern**:
```cpp
// Each effect registers itself
REGISTER_EFFECT(EffectBlur, "Trans / Blur", {
  .param_block_size = sizeof(BlurParams),
  .parse_binary = parseBlurBinary,
});
```

### avs-math

**Purpose**: Vector math and SIMD utilities
**Dependencies**: None
**Features**:
- SSE/AVX optimized routines
- Color space conversions (RGB ↔ HSV)
- Matrix operations

---

## Applications

### avs-player

**Purpose**: Command-line preset player
**Location**: `apps/avs-player/main.cpp`
**Features**:
- Headless rendering (--headless)
- WAV file input (--wav)
- Golden hash generation (--golden-md5)
- Device selection (--input-device)
- Frame limiting (--frames)

**Usage**:
```bash
# Render preset with WAV audio
avs-player --headless --wav music.wav --preset cool.avs --frames 100 --out frames/

# List audio devices
avs-player --list-input-devices

# Golden hash validation
avs-player --preset test.avs --golden-md5 expected.md5
```

### avs-studio

**Purpose**: GUI preset editor (stub)
**Location**: `apps/avs-studio/main.cpp`
**Status**: 84-byte placeholder (Job #21)
**Planned**:
- ImGui-based UI
- Effect tree editor
- Live preview
- Parameter controls

---

## Build System

### CMake Structure

```cmake
CMakeLists.txt              # Root configuration
├── libs/                   # Each lib has CMakeLists.txt
│   ├── avs-core/CMakeLists.txt
│   └── ...
├── apps/
│   ├── avs-player/CMakeLists.txt
│   └── avs-studio/CMakeLists.txt
└── tools/CMakeLists.txt
```

### Build Options

```cmake
option(AVS_BUILD_PLAYER "Build avs-player" ON)
option(AVS_BUILD_TOOLS "Build auxiliary tooling" ON)
option(AVS_ENABLE_SDL3 "Experimental SDL3 backend" OFF)
option(AVS_ENABLE_PIPEWIRE "Pipewire audio (Linux)" ON)
```

### Dependencies

**Required**:
- C++20 compiler (GCC 13+, Clang 15+)
- CMake 3.22+
- SDL2 (windowing, input)
- OpenGL (rendering)

**Optional**:
- PortAudio (audio capture)
- libpipewire-0.3 (Linux audio)
- Google Test (testing)
- ImGui (Studio UI)

**Vendored** (in `libs/third_party/`):
- ns-eel (EEL scripting)
- kiss_fft (FFT implementation)
- stb_image_write (screenshot export)
- dr_wav (WAV file loading)
- nlohmann/json (JSON parsing)

### Build Commands

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)

# Run tests
ctest

# Install
cmake --install . --prefix /usr/local
```

---

## Preset System

### Format Evolution

1. **Text Format** (legacy):
```
blur radius=5
scripted point="x=cos(i);y=sin(i)"
```

2. **Binary .avs** (Winamp AVS):
```
Magic: "Nullsoft AVS Preset 0.2\x1a"
Header: [mode, extended data]
Effects: [effect_id, payload_len, payload_data...]
```

3. **JSON** (modern, Job #18):
```json
{
  "version": "1.0",
  "compat": "strict",
  "effects": [
    {
      "effect": "Trans / Blur",
      "params": [
        {"name": "radius", "type": "int", "value": 5}
      ]
    }
  ]
}
```

### Conversion Flow

```
Binary .avs ──→ avs-compat parser ──→ ParsedPreset (Effect objects)
                      ↓
Text preset ──→ avs-preset parser ──→ IRPreset ──→ avs-convert ──→ JSON
                      ↓
JSON ──────────→ JSON deserializer ──┘
```

---

## Effect System

### Effect Lifecycle

```
1. Registration  → EffectRegistry::registerFactory()
2. Creation      → factory() creates effect instance
3. Configuration → effect->setParams(params)
4. Rendering     → effect->render(context) or smp_render()
5. Destruction   → unique_ptr cleanup
```

### Effect Categories

| Category | Description | Examples |
|----------|-------------|----------|
| **Render** | Generate visuals | Simple, Superscope, Particles |
| **Trans** | Transform framebuffer | Blur, Water, Movement, Mirror |
| **Misc** | Utilities | Comment, BPM, Buffer Save |

### Multi-threaded Rendering (Job #19)

```cpp
// Effect opts into multi-threading
class BlurEffect : public IEffect {
  bool supportsMultiThreaded() const override { return true; }

  bool smp_render(RenderContext& ctx, int tid, int threads) override {
    // Divide framebuffer by rows
    int rowsPerThread = ctx.height / threads;
    int startRow = tid * rowsPerThread;
    int endRow = (tid == threads - 1) ? ctx.height : startRow + rowsPerThread;

    // Process assigned rows
    for (int y = startRow; y < endRow; ++y) {
      applyBlur(ctx.framebuffer, y);
    }
    return true;
  }
};
```

---

## Audio Pipeline

### Capture Flow

```
Audio Device (mic/loopback)
    ↓
PortAudio / Pipewire
    ↓
AudioEngine::poll() - Circular buffer
    ↓
FFT Analysis - Spectrum, beats
    ↓
RenderContext.audioSpectrum
    ↓
Effects consume audio data
```

### Beat Detection

```cpp
struct Analysis {
  float rms;           // Root mean square (volume)
  float peak;          // Peak amplitude
  float bassEnergy;    // Low frequency energy
  bool beat;           // Beat detected this frame
};
```

---

## Rendering Pipeline

### Frame Rendering

```
1. Audio Analysis
   ↓
2. RenderContext creation (framebuffer, audio, beat, RNG)
   ↓
3. Pipeline::render()
   ├─ For each effect:
   │  ├─ effect->render(context)  [single-threaded]
   │  └─ OR threadPool.execute([&](tid, threads) {
   │       effect->smp_render(context, tid, threads);
   │     });
   ↓
4. Framebuffer → OpenGL texture
   ↓
5. Display / Export
```

### Deterministic Rendering

- Seeded RNG per frame (DeterministicRng)
- Consistent thread-local RNG for multi-threading
- Golden hash validation in tests

---

## Testing

### Test Categories

1. **Unit Tests**: Library-level (`libs/*/tests/`)
2. **Integration Tests**: End-to-end (`tests/`)
3. **Golden Hash**: Rendering output validation

### Golden Hash Testing

```bash
# Generate golden hash
avs-player --preset test.avs --wav audio.wav --frames 100 --golden-md5-gen test.md5

# Validate against golden hash
avs-player --preset test.avs --wav audio.wav --frames 100 --golden-md5 test.md5
```

**Purpose**: Detect rendering regressions

### Running Tests

```bash
cd build
ctest                  # All tests
ctest -R avs_core     # Specific test
ctest -V              # Verbose output
```

---

## Development Workflow

### Adding a New Effect

1. **Create effect class**:
```cpp
// libs/avs-effects-legacy/src/trans/effect_my_effect.cpp
#include <avs/effects/effect.hpp>

class MyEffect : public avs::effects::legacy::Effect {
 public:
  bool render(RenderContext& ctx) override {
    // Implement rendering logic
    return true;
  }

  void loadParamsFromBlock(const ParamBlock& block) override {
    param1 = block.getInt("param1", 0);
  }

 private:
  int param1 = 0;
};

REGISTER_EFFECT(MyEffect, "Trans / My Effect", {
  .param_block_size = sizeof(MyEffectParams),
  .parse_binary = parseMyEffectBinary,
});
```

2. **Add to CMakeLists.txt**
3. **Write tests**
4. **Update documentation**

### Debugging

```bash
# Build with debug symbols
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Run under GDB
gdb --args ./avs-player --preset test.avs

# Enable logging
export AVS_LOG_LEVEL=DEBUG
```

### Code Style

- **C++20** features encouraged
- **clang-format** for formatting
- **RAII** for resource management
- **const correctness**
- **Minimal raw pointers** (use smart pointers)

---

## Future Roadmap

See `codex/jobs/README.md` for detailed task breakdown.

**Completed** (Jobs 00-18):
- Build system, audio pipeline, preset parsing, JSON format

**In Progress** (Jobs 19-21):
- Multi-threaded rendering
- Pipewire audio backend
- Studio UI MVP

**Planned** (Jobs 22-24):
- Framebuffer abstraction (Vulkan, video export)
- End-to-end validation
- Registry consolidation

---

## Additional Resources

- [AVS Original Source Notes](avs_original_source_port_notes/README.md)
- [JSON Preset Schema](preset_json_schema.md)
- [Studio UI Architecture](studio_ui_architecture.md)
- [Performance Guide](performance_guide.md) (TODO)

For questions or contributions, see the GitHub repository.
