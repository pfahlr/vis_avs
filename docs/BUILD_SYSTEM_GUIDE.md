# Build System Guide

This guide explains vis_avs's CMake-based build system, dependency management, cross-platform support, and advanced build configurations.

## Table of Contents

1. [CMake Overview](#cmake-overview)
2. [Build Architecture](#build-architecture)
3. [Build Options](#build-options)
4. [Dependency Management](#dependency-management)
5. [Platform-Specific Builds](#platform-specific-builds)
6. [Advanced Configurations](#advanced-configurations)
7. [CI/CD Integration](#cicd-integration)
8. [Packaging and Installation](#packaging-and-installation)
9. [Troubleshooting](#troubleshooting)

## CMake Overview

### Requirements

- **CMake**: 3.22 or later
- **C++ Compiler**: GCC 9+, Clang 10+, or MSVC 2019+
- **C++ Standard**: C++17 (C++20 features used where available)

### Basic Build Commands

```bash
# Standard build
mkdir build && cd build
cmake ..
cmake --build .

# With options
cmake .. -DCMAKE_BUILD_TYPE=Release -DAVS_BUILD_PLAYER=ON
cmake --build . -j$(nproc)

# Install
sudo cmake --install .
```

## Build Architecture

### Directory Structure

```
vis_avs/
├── CMakeLists.txt          # Root build configuration
├── cmake/                  # CMake modules
│   ├── FindPortAudio.cmake
│   ├── FindPipewire.cmake
│   └── CompilerWarnings.cmake
├── libs/                   # Library targets
│   ├── avs-core/
│   │   └── CMakeLists.txt
│   ├── avs-effects-legacy/
│   │   └── CMakeLists.txt
│   └── ...
├── apps/                   # Application targets
│   ├── avs-player/
│   │   └── CMakeLists.txt
│   └── ...
└── tests/                  # Test targets
    └── CMakeLists.txt
```

### Target Organization

vis_avs uses **modular library targets**:

```
avs-player (executable)
├── avs-core (library)
├── avs-effects-legacy (library)
│   ├── avs-core
│   ├── avs-dsl
│   └── avs-math
├── avs-preset (library)
│   └── avs-core
├── avs-audio-io (library)
│   └── PortAudio (external)
└── avs-render-gl (library)
    └── SDL2 (external)
```

### Build Phases

1. **Configure** - CMake generates build files
   ```bash
   cmake .. -DCMAKE_BUILD_TYPE=Release
   ```

2. **Build** - Compile sources
   ```bash
   cmake --build .
   ```

3. **Test** - Run test suite
   ```bash
   ctest
   ```

4. **Install** - Install to system
   ```bash
   cmake --install .
   ```

## Build Options

### Standard Options

| Option | Default | Description |
|--------|---------|-------------|
| `CMAKE_BUILD_TYPE` | Debug | Build type: Debug, Release, RelWithDebInfo, MinSizeRel |
| `CMAKE_INSTALL_PREFIX` | /usr/local | Installation prefix |
| `CMAKE_CXX_COMPILER` | Auto | C++ compiler (g++, clang++, etc.) |
| `BUILD_SHARED_LIBS` | OFF | Build shared libraries instead of static |

**Example:**
```bash
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/opt/vis_avs \
  -DCMAKE_CXX_COMPILER=clang++
```

### AVS-Specific Options

| Option | Default | Description |
|--------|---------|-------------|
| `AVS_BUILD_PLAYER` | ON | Build avs-player application |
| `AVS_BUILD_TOOLS` | ON | Build development tools (avs-convert, gen_golden_md5) |
| `AVS_ENABLE_SDL3` | OFF | Use SDL3 instead of SDL2 (experimental) |
| `AVS_INSTALL_RESOURCES` | ON | Install resource files (palettes, etc.) |
| `AVS_BUILD_AUDIO` | ON | Enable audio capture support |
| `AVS_ENABLE_PIPEWIRE` | OFF | Enable Pipewire audio backend (Linux) |
| `AVS_SKIP_AUTO_DEPS` | OFF | Skip automatic dependency installation |
| `AVS_BUILD_TESTS` | ON | Build test suite |
| `AVS_ENABLE_WARNINGS_AS_ERRORS` | ON | Treat compiler warnings as errors |

**Example:**
```bash
cmake .. \
  -DAVS_BUILD_PLAYER=ON \
  -DAVS_BUILD_TOOLS=ON \
  -DAVS_ENABLE_PIPEWIRE=ON \
  -DAVS_ENABLE_WARNINGS_AS_ERRORS=OFF
```

### Build Type Details

#### Debug
- **Optimizations**: Disabled (`-O0`)
- **Debug symbols**: Enabled (`-g`)
- **Assertions**: Enabled
- **Use case**: Development, debugging

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
```

#### Release
- **Optimizations**: Maximum (`-O3`)
- **Debug symbols**: Disabled
- **Assertions**: Disabled (`-DNDEBUG`)
- **LTO**: Enabled (if supported)
- **Use case**: Production builds

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
```

#### RelWithDebInfo
- **Optimizations**: Moderate (`-O2`)
- **Debug symbols**: Enabled (`-g`)
- **Assertions**: Disabled
- **Use case**: Profiling, production debugging

```bash
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

#### MinSizeRel
- **Optimizations**: Size (`-Os`)
- **Debug symbols**: Disabled
- **Use case**: Embedded systems, minimal footprint

```bash
cmake .. -DCMAKE_BUILD_TYPE=MinSizeRel
```

## Dependency Management

### Required Dependencies

#### SDL2
**Purpose**: Windowing and input handling

**Auto-detection**:
```cmake
find_package(SDL2 2.28 REQUIRED)
```

**Manual specification**:
```bash
cmake .. -DSDL2_DIR=/path/to/sdl2/lib/cmake/SDL2
```

**Fallback**: If system SDL2 < 2.28, CMake downloads and builds SDL 2.28.5 locally.

#### OpenGL
**Purpose**: Graphics rendering

**Auto-detection**:
```cmake
find_package(OpenGL REQUIRED)
```

**Typically provided by**: Mesa (Linux), system libraries (macOS/Windows)

#### PortAudio
**Purpose**: Cross-platform audio capture

**Installation**:
```bash
# Ubuntu
sudo apt-get install portaudio19-dev libportaudio2

# Fedora
sudo dnf install portaudio-devel portaudio
```

**CMake detection**:
```cmake
find_package(PortAudio REQUIRED)
```

### Optional Dependencies

#### Pipewire (Linux only)
**Purpose**: Low-latency audio backend

**Installation**:
```bash
# Ubuntu 22.04+
sudo apt-get install libpipewire-0.3-dev

# Fedora 35+
sudo dnf install pipewire-devel
```

**Enable in build**:
```bash
cmake .. -DAVS_ENABLE_PIPEWIRE=ON
```

#### Google Test
**Purpose**: Unit testing framework

**Installation**:
```bash
# Ubuntu
sudo apt-get install libgtest-dev

# Fedora
sudo dnf install gtest-devel
```

**CMake detection**:
```cmake
find_package(GTest)
if(GTest_FOUND)
  enable_testing()
endif()
```

### Vendored Dependencies

These are included in `libs/third_party/`:

| Library | Purpose | Version |
|---------|---------|---------|
| **ns-eel** | Expression Evaluation Language | Nullsoft (legacy) |
| **kiss_fft** | Fast Fourier Transform | 1.3.1 |
| **nlohmann/json** | JSON parsing | 3.11.2 |
| **stb_image** | Image loading | 2.28 |
| **stb_image_write** | Image writing | 1.16 |
| **dr_wav** | WAV file loading | 0.13.8 |
| **sha256** | SHA-256 hashing | Custom |

### Automatic Dependency Installation

CMake automatically runs `tools/ensure_build_dependencies.sh` to install missing packages on Ubuntu/Fedora.

**Disable**:
```bash
cmake .. -DAVS_SKIP_AUTO_DEPS=1
```

**Script behavior**:
- Detects missing development packages
- Runs `apt-get` or `dnf` with `sudo`
- Installs only necessary packages
- Skips if running as root in container

## Platform-Specific Builds

### Linux

#### Ubuntu/Debian

**Install dependencies**:
```bash
./run_setup_dev_environment.sh --platform ubuntu
```

**Manual install**:
```bash
sudo apt-get install -y \
  cmake g++ clang-format git pkg-config \
  libsdl2-dev mesa-common-dev libglu1-mesa-dev \
  portaudio19-dev libportaudio2 libgtest-dev \
  libsamplerate0-dev libjack-dev libasound2-dev
```

**Build**:
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

#### Fedora/RHEL

**Install dependencies**:
```bash
./run_setup_dev_environment.sh --platform fedora
```

**Manual install**:
```bash
sudo dnf install -y \
  cmake g++ clang-tools-extra git pkgconfig \
  SDL2-devel mesa-libGL-devel mesa-libGLU-devel \
  portaudio-devel portaudio gtest-devel \
  jack-audio-connection-kit-devel alsa-lib-devel
```

**Build**: Same as Ubuntu

#### Arch Linux

**Install dependencies**:
```bash
sudo pacman -S cmake gcc clang git pkgconfig \
  sdl2 mesa glu portaudio gtest jack2 alsa-lib
```

### Windows (Planned)

#### MinGW-w64

**Status**: Planned (Job #TBD)

**Dependencies**: Install via MSYS2
```bash
pacman -S mingw-w64-x86_64-cmake \
          mingw-w64-x86_64-gcc \
          mingw-w64-x86_64-SDL2 \
          mingw-w64-x86_64-portaudio
```

**Build**:
```bash
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

#### Visual Studio

**Status**: Planned

**Dependencies**: vcpkg
```powershell
vcpkg install sdl2:x64-windows portaudio:x64-windows
```

**Build**:
```powershell
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

### macOS (Planned)

**Status**: Planned (Job #TBD)

**Dependencies**: Homebrew
```bash
brew install cmake sdl2 portaudio googletest
```

**Build**:
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(sysctl -n hw.ncpu)
```

**Metal backend**: Planned for native rendering

## Advanced Configurations

### Sanitizer Builds

#### Address Sanitizer (ASan)
Detects memory errors (buffer overflows, use-after-free, leaks):

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer -g"
cmake --build .

# Run
./apps/avs-player/avs-player --preset test.avs
```

#### Undefined Behavior Sanitizer (UBSan)
Detects undefined behavior:

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=undefined -fno-omit-frame-pointer -g"
cmake --build .
```

#### Thread Sanitizer (TSan)
Detects data races:

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=thread -g"
cmake --build .
```

#### Combined Sanitizers
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -g"
```

### Link-Time Optimization (LTO)

Enable for maximum performance:

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON
cmake --build .
```

**Benefits**: 5-15% performance improvement
**Drawbacks**: Slower build times, larger memory usage

### Static Linking

Build fully static binaries:

```bash
cmake .. -DBUILD_SHARED_LIBS=OFF \
  -DCMAKE_EXE_LINKER_FLAGS="-static-libgcc -static-libstdc++"
cmake --build .
```

### Cross-Compilation

#### ARM64 on x86_64

```bash
cmake .. \
  -DCMAKE_SYSTEM_NAME=Linux \
  -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
  -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
  -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++ \
  -DCMAKE_FIND_ROOT_PATH=/usr/aarch64-linux-gnu
```

### Verbose Build Output

See full compiler commands:

```bash
cmake --build . --verbose

# Or with make
make VERBOSE=1
```

### Parallel Builds

```bash
# Auto-detect cores
cmake --build . -j$(nproc)

# Specify core count
cmake --build . -j8
```

### Compiler Cache (ccache)

Speed up rebuilds:

```bash
# Install
sudo apt-get install ccache

# Configure
cmake .. -DCMAKE_CXX_COMPILER_LAUNCHER=ccache

# Build (cached)
cmake --build .
```

## CI/CD Integration

### GitHub Actions

`.github/workflows/ci.yml`:

```yaml
name: CI

on: [push, pull_request]

jobs:
  build-linux:
    runs-on: ubuntu-22.04
    steps:
      - uses: actions/checkout@v3

      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y cmake g++ libsdl2-dev \
            portaudio19-dev libgtest-dev

      - name: Configure
        run: |
          mkdir build && cd build
          cmake .. -DCMAKE_BUILD_TYPE=Release

      - name: Build
        run: cmake --build build -j$(nproc)

      - name: Test
        run: |
          cd build
          ctest --output-on-failure
```

### Docker Builds

**Ubuntu 22.04**:
```dockerfile
FROM ubuntu:22.04
RUN apt-get update && apt-get install -y \
    cmake g++ git libsdl2-dev portaudio19-dev \
    libgtest-dev mesa-common-dev libglu1-mesa-dev
COPY . /vis_avs
WORKDIR /vis_avs/build
RUN cmake .. -DCMAKE_BUILD_TYPE=Release && \
    cmake --build . -j$(nproc) && \
    ctest
```

**Build**:
```bash
docker build -t vis_avs .
docker run -v $(pwd)/output:/output vis_avs \
  cp /vis_avs/build/apps/avs-player/avs-player /output/
```

## Packaging and Installation

### System Installation

```bash
# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build .

# Install
sudo cmake --install .
```

**Installed files**:
```
/usr/local/
├── bin/
│   ├── avs-player
│   └── avs-convert
├── lib/vis_avs/
│   ├── libavs-core.a
│   ├── libavs-effects-legacy.a
│   └── ...
├── include/vis_avs/
│   ├── avs/core/IEffect.hpp
│   └── ...
└── share/vis_avs/
    └── resources/
        └── palettes/
```

### Uninstall

```bash
cd build
sudo cmake --build . --target uninstall
```

Or manually:
```bash
sudo rm -rf /usr/local/bin/avs-*
sudo rm -rf /usr/local/lib/vis_avs
sudo rm -rf /usr/local/include/vis_avs
sudo rm -rf /usr/local/share/vis_avs
```

### DEB Package (Debian/Ubuntu)

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
cpack -G DEB

# Install
sudo dpkg -i vis_avs-*.deb
```

### RPM Package (Fedora/RHEL)

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
cpack -G RPM

# Install
sudo rpm -i vis_avs-*.rpm
```

### AppImage (Universal Linux)

**Status**: Planned

```bash
./tools/create_appimage.sh
```

## Troubleshooting

### SDL2 Version Too Old

**Error**:
```
CMake Error: Could not find SDL2 >= 2.28
```

**Solution 1** (Auto-download):
```bash
cmake ..  # Auto-downloads SDL 2.28.5
```

**Solution 2** (Manual install):
```bash
# Download SDL2 2.28+
wget https://github.com/libsdl-org/SDL/releases/download/release-2.28.5/SDL2-2.28.5.tar.gz
tar xf SDL2-2.28.5.tar.gz
cd SDL2-2.28.5
./configure --prefix=/usr/local
make -j$(nproc)
sudo make install
```

### PortAudio Not Found

**Error**:
```
CMake Error: Could not find PortAudio
```

**Solution**:
```bash
# Ubuntu
sudo apt-get install portaudio19-dev libportaudio2

# Fedora
sudo dnf install portaudio-devel portaudio
```

### OpenGL Not Found

**Error**:
```
CMake Error: Could not find OpenGL
```

**Solution**:
```bash
# Ubuntu
sudo apt-get install mesa-common-dev libglu1-mesa-dev

# Fedora
sudo dnf install mesa-libGL-devel mesa-libGLU-devel
```

### Compiler Warnings as Errors

**Error**:
```
error: unused variable 'foo' [-Werror=unused-variable]
```

**Solution 1** (Fix warning):
```cpp
// Comment out or use the variable
```

**Solution 2** (Disable warnings-as-errors):
```bash
cmake .. -DAVS_ENABLE_WARNINGS_AS_ERRORS=OFF
```

### Out of Memory During Build

**Error**:
```
c++: fatal error: Killed signal terminated program
```

**Solution** (Reduce parallelism):
```bash
# Use fewer cores
cmake --build . -j2

# Or disable parallel builds
cmake --build .
```

### Missing Pipewire Headers

**Error**:
```
CMake Error: Could not find Pipewire
```

**Solution**:
```bash
# Ubuntu 22.04+
sudo apt-get install libpipewire-0.3-dev

# Or disable Pipewire
cmake .. -DAVS_ENABLE_PIPEWIRE=OFF
```

### Tests Fail to Link

**Error**:
```
undefined reference to `testing::...'
```

**Solution**:
```bash
# Ubuntu - install Google Test
sudo apt-get install libgtest-dev

# Or disable tests
cmake .. -DAVS_BUILD_TESTS=OFF
```

## Best Practices

### Development Builds
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug \
  -DAVS_ENABLE_WARNINGS_AS_ERRORS=ON \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON  # For clangd
```

### Release Builds
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON
```

### Sanitizer Testing
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined"
```

### CI Builds
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release \
  -DAVS_ENABLE_WARNINGS_AS_ERRORS=ON \
  -DAVS_BUILD_TESTS=ON
```

## Resources

- **CMake Documentation**: https://cmake.org/documentation/
- **CMake Examples**: `cmake/` directory
- **Build Scripts**: `run_build.sh`, `run_setup_dev_environment.sh`
- **CI Configuration**: `.github/workflows/ci.yml`

## Next Steps

- Read [DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md) for development workflow
- Check [CODEBASE_ARCHITECTURE.md](CODEBASE_ARCHITECTURE.md) for system design
- Browse `codex/jobs/` for build system improvement tasks

Happy building! 🔨
