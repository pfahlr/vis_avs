# VST Plugin Implementation - Quick Reference

Quick reference guide for developers working on the VST plugin build target.

## Task Order (Critical Path)

```
25. Framework Integration  →  26. Parameter Bank  →  28. Preset Mapping
    ↓                                                        ↓
27. Audio Bridge          →                         29. Engine Integration
                                                            ↓
                                                   32. Testing & Validation
                                                            ↓
                                                   33. Packaging & Distribution
```

**Optional**: Task 30 (MIDI), Task 31 (Renderer Hub) can be added after Task 29.

## Key Files & Directories

### New Directories to Create

```
libs/avs-vst-plugin/          # Core plugin library
├── include/avs/vst/
│   ├── PluginProcessor.hpp
│   ├── ParameterBank.hpp
│   ├── AudioFeatureBridge.hpp
│   ├── ControlMappingTable.hpp
│   └── ...
└── src/
    └── ...

apps/avs-vst3/                # VST3 plugin entry point
├── PluginEditor.h/cpp
├── OpenGLRenderer.cpp
└── CMakeLists.txt

libs/third_party/juce/        # JUCE framework (FetchContent)

tests/vst/                    # VST-specific tests
├── test_parameter_bank.cpp
├── test_audio_bridge.cpp
└── ...

installers/                   # Platform installers
├── windows/avs_vst3.nsi
├── macos/create_dmg.sh
└── linux/debian/
```

## CMake Configuration

### Enable VST3 Build

```bash
cmake -B build -DAVS_BUILD_VST3=ON
cmake --build build --target avs_vst3
```

### JUCE Integration (in `cmake/JUCEConfig.cmake`)

```cmake
include(FetchContent)

FetchContent_Declare(
  JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 7.0.12
)

FetchContent_MakeAvailable(JUCE)
```

## Data Structures

### Parameter Definition

```cpp
constexpr size_t kNumMacros = 32;
constexpr size_t kNumButtons = 8;
constexpr size_t kNumXYPads = 2;
constexpr size_t kNumScenes = 8;
constexpr size_t kTotalParameters = 56;

enum class ParamID : uint32_t {
    Macro1 = 0,
    // ... Macro32 = 31,
    Button1 = 32,
    // ... Button8 = 39,
    XYPad1_X = 40,
    XYPad1_Y = 41,
    XYPad2_X = 42,
    XYPad2_Y = 43,
    Scene1 = 44,
    // ... Scene8 = 51,
    Crossfader = 52,
    PresetNext = 53,
    PresetPrev = 54,
};
```

### Audio Features Structure

```cpp
struct AudioFeatures {
    std::array<float, 256> spectrum;
    std::array<float, 128> waveform;
    float beatIntensity;
    float bassEnergy, midEnergy, highEnergy;
    uint64_t timestamp;
};
```

### Preset Control Mapping

```cpp
struct ControlMapping {
    std::string controlID;      // "macro.1"
    std::string targetPath;     // "effect.superscope.n_points"
    float rangeMin, rangeMax;
    CurveType curve;
    float smoothingMs;
};
```

## Threading Rules

### Audio Thread (Real-time)
✅ **Allowed:**
- Read parameters (lock-free)
- Extract audio features
- Push to lock-free queue
- Pre-allocated buffers

❌ **Forbidden:**
- `new`, `delete`, `malloc`, `free`
- `std::mutex`, `std::condition_variable`
- File I/O, network I/O
- `std::cout`, `std::cerr` (use lock-free logger)

### Render Thread (60 FPS)
✅ **Allowed:**
- OpenGL calls
- Load presets
- Update engine state
- Pop from lock-free queue

❌ **Avoid:**
- Long-running operations (>10ms)
- Blocking on mutexes

### UI Thread
✅ **Allowed:**
- Display frames
- User input
- UI updates

❌ **Forbidden:**
- Direct engine manipulation (use message queue)

## Lock-Free Queue Template

```cpp
#include <boost/lockfree/spsc_queue.hpp>

// In header
boost::lockfree::spsc_queue<AudioFeatures> featureQueue{128};

// Audio thread (producer)
AudioFeatures features = extractFeatures(...);
if (!featureQueue.push(features)) {
    // Queue full, drop frame (acceptable for vis)
}

// Render thread (consumer)
AudioFeatures features;
while (featureQueue.pop(features)) {
    engine.updateFeatures(features);
}
```

## Parameter Smoothing

```cpp
class ParameterSmoother {
    float current = 0.0f;
    float target = 0.0f;
    float coeff = 0.1f;  // Set via setTimeConstant()

public:
    void setTarget(float t) { target = t; }

    float getNextValue() {
        current += coeff * (target - current);
        return current;
    }

    void setTimeConstant(float ms, float sampleRate) {
        coeff = 1.0f - std::exp(-1.0f / (ms * 0.001f * sampleRate));
    }
};
```

## JUCE Plugin Skeleton

```cpp
class AVSPluginProcessor : public juce::AudioProcessor {
public:
    void prepareToPlay(double sampleRate, int samplesPerBlock) override {
        // Initialize feature extractors, allocate buffers
    }

    void processBlock(juce::AudioBuffer<float>& buffer,
                       juce::MidiBuffer& midi) override {
        // 1. Passthrough audio
        // 2. Extract features
        // 3. Push to queue
    }

    juce::AudioProcessorEditor* createEditor() override {
        return new AVSPluginEditor(*this);
    }

    void getStateInformation(juce::MemoryBlock& destData) override {
        // Serialize state to JSON
    }

    void setStateInformation(const void* data, int sizeInBytes) override {
        // Deserialize state from JSON
    }
};
```

## Testing Checklist

### Unit Tests
- [ ] Parameter smoothing (test_parameter_bank.cpp)
- [ ] Ring buffer thread safety (test_ring_buffer.cpp)
- [ ] Curve evaluation (test_control_mapping.cpp)
- [ ] MIDI CC mapping (test_midi_cc.cpp)

### Integration Tests
- [ ] Load preset → verify visualization
- [ ] DAW automation → verify parameter updates
- [ ] MIDI CC learn → verify controller mapping
- [ ] State save/restore → verify persistence

### Validation
- [ ] pluginval strict mode (0 errors)
- [ ] Load in Reaper (Linux/Windows/macOS)
- [ ] Load in Bitwig (Linux)
- [ ] Load in Ableton (Windows/macOS)
- [ ] Load in FL Studio (Windows)

### Performance
- [ ] CPU usage <5% per instance
- [ ] Frame rate stable at 60 FPS
- [ ] No allocations in processBlock() (verify with Valgrind)

## Common Pitfalls

### 1. Allocations in Audio Thread
```cpp
// ❌ BAD: Allocation in processBlock()
std::vector<float> temp(numSamples);

// ✅ GOOD: Pre-allocate in prepareToPlay()
std::vector<float> tempBuffer;  // Member variable
void prepareToPlay(...) {
    tempBuffer.resize(maxSamples);
}
```

### 2. Blocking in Audio Thread
```cpp
// ❌ BAD: Mutex in processBlock()
std::lock_guard<std::mutex> lock(mutex);

// ✅ GOOD: Lock-free queue
featureQueue.push(features);
```

### 3. OpenGL Context Threading
```cpp
// ❌ BAD: OpenGL call without context
glBindTexture(...);  // Crash!

// ✅ GOOD: Make context current first
openglContext.makeActive();
glBindTexture(...);
openglContext.release();
```

### 4. Parameter ID Stability
```cpp
// ❌ BAD: Changing parameter count on preset load
void loadPreset() {
    clearAllParameters();
    addParametersFromPreset();  // Breaks DAW automation!
}

// ✅ GOOD: Fixed parameter count, update mappings
void loadPreset() {
    mappingTable.clear();
    mappingTable.loadFromPreset();  // Same 56 params, new mappings
}
```

## Build Commands

### Linux
```bash
cmake -B build -DAVS_BUILD_VST3=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 8
```

### macOS
```bash
cmake -B build -DAVS_BUILD_VST3=ON -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"
cmake --build build --parallel 8
```

### Windows (MSVC)
```powershell
cmake -B build -DAVS_BUILD_VST3=ON -G "Visual Studio 17 2022"
cmake --build build --config Release
```

## Debugging Tips

### Linux (GDB)
```bash
# Run plugin in Reaper under GDB
gdb --args reaper

# Catch exceptions
(gdb) catch throw
(gdb) run
```

### macOS (lldb)
```bash
lldb -- /Applications/Reaper.app/Contents/MacOS/REAPER
(lldb) run
```

### Profiling
```bash
# Linux: CPU profiling
perf record -g ./reaper
perf report

# Linux: Memory profiling
valgrind --tool=massif ./reaper
ms_print massif.out.12345

# macOS: Instruments
instruments -t "Time Profiler" /Applications/Reaper.app
```

## Resources

- **JUCE Docs**: https://juce.com/learn/documentation
- **VST3 SDK**: https://steinbergmedia.github.io/vst3_dev_portal/
- **pluginval**: https://github.com/Tracktion/pluginval
- **Plugin Development Forum**: https://forum.juce.com/

## Quick Links to Task Files

- [Task 25: Framework Integration](25-vst-framework-integration.yaml)
- [Task 26: Parameter Bank](26-vst-stable-parameter-bank.yaml)
- [Task 27: Audio Bridge](27-vst-audio-feature-bridge.yaml)
- [Task 28: Preset Mapping](28-vst-preset-control-mapping.yaml)
- [Task 29: Engine Integration](29-vst-engine-integration.yaml)
- [Task 30: MIDI & Sync](30-vst-midi-host-sync.yaml)
- [Task 31: Renderer Hub](31-vst-renderer-hub-ipc.yaml)
- [Task 32: Testing](32-vst-testing-validation.yaml)
- [Task 33: Packaging](33-vst-packaging-distribution.yaml)

---

**Happy coding!** 🎵🎨
