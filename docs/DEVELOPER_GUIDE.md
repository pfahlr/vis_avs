# Developer Guide

Welcome to the vis_avs project! This guide will help you get started as a contributor to this modern C++ reimplementation of Winamp's Advanced Visualization Studio.

## Table of Contents

1. [Quick Start](#quick-start)
2. [Project Overview](#project-overview)
3. [Development Environment Setup](#development-environment-setup)
4. [Building and Testing](#building-and-testing)
5. [Codebase Navigation](#codebase-navigation)
6. [Development Workflow](#development-workflow)
7. [Debugging Tips](#debugging-tips)
8. [Performance Profiling](#performance-profiling)
9. [Common Tasks](#common-tasks)

## Quick Start

Get up and running in under 5 minutes:

```bash
# Clone the repository
git clone https://github.com/pfahlr/vis_avs.git
cd vis_avs

# Install dependencies (Ubuntu/Debian)
./run_setup_dev_environment.sh --platform ubuntu

# Build the project
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j$(nproc)

# Run tests
ctest

# Try the player
./apps/avs-player/avs-player --preset ../tests/data/simple.avs
```

## Project Overview

### What is vis_avs?

vis_avs is a cross-platform reimplementation of Winamp's Advanced Visualization Studio (AVS), a powerful music visualization plugin from the late 1990s/early 2000s. Our goal is to:

- **Modernize**: Port the codebase to modern C++17/20
- **Cross-platform**: Support Linux, Windows, and macOS
- **Preserve**: Maintain binary compatibility with original .avs preset files
- **Extend**: Add new features like multi-threading, modern audio backends
- **Document**: Create comprehensive documentation for community participation

### Project Status

**Current State (as of November 2025):**
- ✅ **56/56 original effects implemented** (100% coverage)
- ✅ Complete audio pipeline (PortAudio + FFT + beat detection)
- ✅ Deterministic rendering with golden hash testing
- ✅ Binary preset parser (full .avs compatibility)
- ✅ JSON preset format support
- ✅ Multi-threaded rendering infrastructure
- 🚧 Studio UI (architecture complete, implementation planned)
- 🚧 Pipewire audio backend (API complete, implementation stub)

### Key Technologies

- **Language**: C++17/20
- **Build System**: CMake 3.22+
- **Windowing**: SDL2 (SDL3 experimental)
- **Graphics**: OpenGL 3.3+
- **Audio**: PortAudio, Pipewire (planned)
- **Scripting**: ns-eel (Expression Evaluation Language)
- **Testing**: Google Test
- **JSON**: nlohmann/json
- **FFT**: Kiss FFT

## Development Environment Setup

### Supported Platforms

| Platform | Status | Notes |
|----------|--------|-------|
| **Linux** | ✅ Primary | Ubuntu 20.04+, Fedora 35+ |
| **Windows** | 🚧 Planned | MinGW/MSVC support planned |
| **macOS** | 🚧 Planned | Metal backend planned |

### System Requirements

**Minimum:**
- CPU: x86_64 with SSE2
- RAM: 2GB
- GPU: OpenGL 3.3 support
- Disk: 500MB for build artifacts

**Recommended:**
- CPU: 4+ cores (for multi-threaded rendering)
- RAM: 4GB+
- GPU: OpenGL 4.5+
- Disk: 1GB

### Required Dependencies

#### Ubuntu/Debian

```bash
sudo apt-get install -y \
  cmake g++ clang-format git pkg-config \
  libsdl2-dev mesa-common-dev libglu1-mesa-dev \
  portaudio19-dev libportaudio2 libgtest-dev \
  libsamplerate0-dev libjack-dev libasound2-dev
```

#### Fedora/RHEL

```bash
sudo dnf install -y \
  cmake g++ clang-tools-extra git pkgconfig \
  SDL2-devel mesa-libGL-devel mesa-libGLU-devel \
  portaudio-devel portaudio gtest-devel \
  jack-audio-connection-kit-devel alsa-lib-devel
```

### Optional Dependencies

- **Pipewire** (Linux): Low-latency audio backend
  ```bash
  sudo apt-get install libpipewire-0.3-dev  # Ubuntu
  sudo dnf install pipewire-devel            # Fedora
  ```

- **clang-tidy**: Static analysis
  ```bash
  sudo apt-get install clang-tidy
  ```

- **valgrind**: Memory leak detection
  ```bash
  sudo apt-get install valgrind
  ```

### IDE Setup

#### VS Code (Recommended)

Install these extensions:
- **C/C++** (ms-vscode.cpptools)
- **CMake Tools** (ms-vscode.cmake-tools)
- **clangd** (llvm-vs-code-extensions.vscode-clangd)

**Workspace settings** (`.vscode/settings.json`):
```json
{
  "cmake.buildDirectory": "${workspaceFolder}/build",
  "cmake.configureSettings": {
    "CMAKE_BUILD_TYPE": "Debug",
    "CMAKE_EXPORT_COMPILE_COMMANDS": "ON"
  },
  "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools",
  "clangd.arguments": [
    "--compile-commands-dir=${workspaceFolder}/build"
  ]
}
```

#### CLion

1. Open project directory
2. CLion will auto-detect CMakeLists.txt
3. Set build type to Debug
4. Enable clang-tidy: **Settings → Editor → Inspections → C/C++ → Clang-Tidy**

## Building and Testing

### Build Types

```bash
# Debug build (default) - enables assertions, debug symbols
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Release build - optimizations enabled
cmake .. -DCMAKE_BUILD_TYPE=Release

# Release with debug info
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo

# Sanitizer builds (for development)
cmake .. -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address -fsanitize=undefined"
```

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `AVS_BUILD_PLAYER` | ON | Build avs-player application |
| `AVS_BUILD_TOOLS` | ON | Build development tools |
| `AVS_ENABLE_SDL3` | OFF | Use SDL3 instead of SDL2 |
| `AVS_INSTALL_RESOURCES` | ON | Install resource files |
| `AVS_ENABLE_PIPEWIRE` | OFF | Enable Pipewire backend |
| `AVS_SKIP_AUTO_DEPS` | OFF | Skip automatic dependency installation |

**Example:**
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug \
  -DAVS_BUILD_PLAYER=ON \
  -DAVS_BUILD_TOOLS=ON \
  -DAVS_ENABLE_PIPEWIRE=ON
```

### Incremental Builds

```bash
# Build only changed targets
cmake --build . -j$(nproc)

# Build specific target
cmake --build . --target avs-player

# Clean build
cmake --build . --target clean
```

### Running Tests

```bash
# Run all tests
ctest

# Run tests with output
ctest --output-on-failure

# Run specific test
ctest -R test_blur

# Run tests in parallel
ctest -j$(nproc)

# Run with valgrind (memory leak detection)
ctest -T memcheck
```

### Golden Hash Testing

The project uses MD5 checksums of rendered frames to detect rendering regressions:

```bash
# Regenerate golden hashes after intentional changes
./tools/gen_golden_md5 --frames 10 --width 320 --height 240 --seed 1234 \
  --preset ../tests/regression/data/tiny_preset_fragment.avs \
  > ../tests/regression/data/expected_md5_320x240_seed1234.json

# Validate
ctest -R offscreen_golden_md5_snapshot
```

## Codebase Navigation

### Directory Structure

```
vis_avs/
├── apps/               # Applications
│   ├── avs-player/    # CLI player (821 lines)
│   └── avs-studio/    # GUI editor (planned)
│
├── libs/              # Modular libraries (14 total)
│   ├── avs-core/      # Effect pipeline & registry
│   ├── avs-effects-legacy/  # Original 56 effects
│   ├── avs-preset/    # Preset parser
│   ├── avs-audio-*/   # Audio backends
│   └── ...
│
├── tests/             # Test suite (48 test files)
│   ├── core/         # Unit tests
│   ├── regression/   # Golden hash tests
│   └── data/         # Test presets
│
├── tools/            # Developer utilities
│   ├── avs-convert   # Preset format converter
│   └── gen_golden_md5  # Hash generator
│
├── docs/             # Documentation
│   ├── CODEBASE_ARCHITECTURE.md
│   ├── effects/      # Per-effect docs
│   └── ...
│
├── codex/            # Development roadmap
│   └── jobs/         # 25 YAML job specs
│
└── resources/        # Runtime assets
    └── palettes/     # Color palettes
```

### Key Files to Know

**Core Architecture:**
- `libs/avs-core/include/avs/core/IEffect.hpp` - Effect interface
- `libs/avs-core/include/avs/core/Pipeline.hpp` - Effect pipeline
- `libs/avs-core/include/avs/core/RenderContext.hpp` - Rendering state
- `libs/avs-core/include/avs/core/EffectRegistry.hpp` - Factory pattern

**Effect Implementation:**
- `libs/avs-effects-legacy/include/avs/effects/` - Effect headers
- `libs/avs-effects-legacy/src/<category>/effect_*.cpp` - Implementations

**Audio System:**
- `libs/avs-audio-io/include/avs/audio/AudioInput.hpp` - Audio capture
- `libs/avs-audio-dsp/include/avs/audio/fft.hpp` - FFT analysis

**Applications:**
- `apps/avs-player/main.cpp` - Player implementation (821 lines)

### Finding Your Way Around

**Use these commands to explore:**

```bash
# Find effect implementations
find libs/avs-effects-legacy/src -name "effect_*.cpp"

# Search for specific functionality
grep -r "class.*Effect" libs/avs-core/include/

# Find test files
find tests -name "*.cpp" -type f

# List all job specifications
ls codex/jobs/
```

## Development Workflow

### 1. Choose a Task

Browse open tasks in `codex/jobs/`:

```bash
# View job list
ls -l codex/jobs/

# Read a job spec
cat codex/jobs/19-render-multithreaded-effects.yaml
```

Jobs are organized by status:
- `status: TODO` - Not started
- `status: IN_PROGRESS` - Active work
- `status: DONE` - Completed

### 2. Create a Feature Branch

```bash
git checkout -b feature/your-feature-name
```

### 3. Make Changes

Follow these principles:
- **One concern per commit** - Keep commits focused
- **Write tests first** - TDD when possible
- **Document as you go** - Update docs with code changes
- **Run tests frequently** - `ctest` after changes
- **Use clang-format** - Auto-format code

### 4. Test Your Changes

```bash
# Build
cmake --build build -j$(nproc)

# Run relevant tests
cd build
ctest -R your_test_pattern

# Run all tests
ctest

# Manual testing
./apps/avs-player/avs-player --preset ../tests/data/your_test.avs
```

### 5. Commit and Push

```bash
# Format code
clang-format -i path/to/your/file.cpp

# Add changes
git add .

# Commit with descriptive message
git commit -m "feat: add multi-threaded blur rendering

- Implement row-based work distribution
- Add thread-local prefix arrays
- 2x speedup on 4-core systems"

# Push to remote
git push origin feature/your-feature-name
```

### 6. Create Pull Request

1. Open GitHub and create PR
2. Fill in PR template
3. Wait for CI to pass
4. Address review comments
5. Merge when approved

## Debugging Tips

### Using GDB

```bash
# Build with debug symbols
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .

# Run under GDB
gdb --args ./apps/avs-player/avs-player --preset test.avs

# Common GDB commands
(gdb) break Pipeline::render    # Set breakpoint
(gdb) run                        # Start program
(gdb) next                       # Step over
(gdb) step                       # Step into
(gdb) print context.frameIndex  # Print variable
(gdb) backtrace                 # Show call stack
```

### Address Sanitizer

Catch memory errors and undefined behavior:

```bash
# Build with ASan
cmake .. -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address -fsanitize=undefined -g"
cmake --build .

# Run (will abort on errors)
./apps/avs-player/avs-player --preset test.avs
```

### Valgrind

Detect memory leaks:

```bash
# Run with valgrind
valgrind --leak-check=full --show-leak-kinds=all \
  ./apps/avs-player/avs-player --preset test.avs --frames 10
```

### Logging

Add debug output:

```cpp
#include <iostream>

void MyEffect::render(RenderContext& context) {
  std::cerr << "Frame " << context.frameIndex
            << " size: " << context.width << "x" << context.height
            << std::endl;
  // ...
}
```

## Performance Profiling

### Perf (Linux)

```bash
# Record performance data
perf record -g ./apps/avs-player/avs-player --preset test.avs --frames 1000

# View report
perf report

# Generate flamegraph
perf script | stackcollapse-perf.pl | flamegraph.pl > flame.svg
```

### CPU Profiling with gprof

```bash
# Build with profiling
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-pg"
cmake --build .

# Run program (generates gmon.out)
./apps/avs-player/avs-player --preset test.avs --frames 1000

# Analyze
gprof ./apps/avs-player/avs-player gmon.out > analysis.txt
```

### Benchmarking

```bash
# Time execution
time ./apps/avs-player/avs-player --preset test.avs --frames 1000 --headless

# Compare before/after
hyperfine 'old-binary --preset test.avs --frames 100' \
          'new-binary --preset test.avs --frames 100'
```

## Common Tasks

### Adding a New Effect

See [EFFECT_DEVELOPMENT_GUIDE.md](EFFECT_DEVELOPMENT_GUIDE.md) for detailed instructions.

Quick overview:

1. Create header: `libs/avs-effects-legacy/include/avs/effects/category/effect_name.h`
2. Create source: `libs/avs-effects-legacy/src/category/effect_name.cpp`
3. Implement `IEffect` interface
4. Register with `AVS_LEGACY_REGISTER_EFFECT`
5. Add CMakeLists.txt entry
6. Write tests
7. Update documentation

### Regenerating Golden Hashes

After intentional rendering changes:

```bash
cd build
./tools/gen_golden_md5 --frames 10 --width 320 --height 240 --seed 1234 \
  --preset ../tests/regression/data/tiny_preset_fragment.avs \
  > ../tests/regression/data/expected_md5_320x240_seed1234.json
```

### Converting Preset Formats

```bash
# Binary to JSON
./tools/avs-convert --input preset.avs --output preset.json

# JSON to binary
./tools/avs-convert --input preset.json --output preset.avs
```

### Listing Audio Devices

```bash
./apps/avs-player/avs-player --list-input-devices
```

### Rendering to File

```bash
# Headless rendering with WAV input
./apps/avs-player/avs-player \
  --headless \
  --wav ../tests/data/test.wav \
  --preset preset.avs \
  --frames 120 \
  --out output_dir
```

### Running Specific Tests

```bash
# Pattern matching
ctest -R blur

# Verbose output
ctest -R blur -V

# List available tests
ctest -N
```

## Next Steps

- Read [CODEBASE_ARCHITECTURE.md](CODEBASE_ARCHITECTURE.md) for architectural overview
- Read [EFFECT_DEVELOPMENT_GUIDE.md](EFFECT_DEVELOPMENT_GUIDE.md) to learn effect development
- Check [CONTRIBUTING.md](../CONTRIBUTING.md) for contribution guidelines
- Browse `codex/jobs/` for available tasks
- Join discussions on GitHub Issues

## Getting Help

- **Documentation**: Check `docs/` directory
- **GitHub Issues**: Ask questions or report bugs
- **Code Comments**: Most complex code is well-commented
- **Git History**: Use `git log` and `git blame` to understand changes

Welcome to the project! We're excited to have you contribute! 🎵🎨
