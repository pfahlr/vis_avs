# VST Plugin Technical Architecture

This document provides deep technical details on the VST plugin implementation, including threading models, memory management, performance optimization, and key algorithms.

## Table of Contents

1. [Threading Model](#threading-model)
2. [Parameter System](#parameter-system)
3. [Audio Path](#audio-path)
4. [Preset Mapping](#preset-mapping)
5. [Render Pipeline](#render-pipeline)
6. [MIDI Processing](#midi-processing)
7. [State Management](#state-management)
8. [Performance Optimization](#performance-optimization)
9. [Error Handling](#error-handling)
10. [Platform-Specific Considerations](#platform-specific-considerations)

---

## Threading Model

### Overview

The plugin uses a **three-thread architecture** to separate concerns and maintain real-time safety:

```
┌────────────────────────────────────────────────────────────────┐
│                          Plugin Process                         │
├────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────┐ │
│  │  Audio Thread    │  │  Render Thread   │  │  UI Thread   │ │
│  │  (Real-time)     │  │  (60 FPS)        │  │  (Event)     │ │
│  ├──────────────────┤  ├──────────────────┤  ├──────────────┤ │
│  │ Priority: 85     │  │ Priority: 50     │  │ Priority: 30 │ │
│  │ Affinity: Core 0 │  │ Affinity: Any    │  │ Affinity: Any│ │
│  │ Stack: 256KB     │  │ Stack: 2MB       │  │ Stack: 2MB   │ │
│  └────────┬─────────┘  └────────┬─────────┘  └──────┬───────┘ │
│           │                     │                    │         │
│           │  Lock-free Queue    │  Double Buffer     │         │
│           └────────────────────>│<───────────────────┘         │
│                                 │                              │
└─────────────────────────────────┼──────────────────────────────┘
                                  │
                          ┌───────▼────────┐
                          │  OpenGL Context│
                          │  (Shared)      │
                          └────────────────┘
```

### Audio Thread

**Responsibilities:**
- Process audio in `processBlock()` callback
- Extract features (FFT, beat detection, waveform)
- Read smoothed parameter values
- Parse incoming MIDI messages
- Push features to lock-free queue

**Real-time constraints:**
- **No allocations** (pre-allocate all buffers in `prepareToPlay()`)
- **No blocking calls** (no mutexes, no I/O, no system calls)
- **Bounded execution time** (<50% of buffer time, e.g., <5ms at 512 samples/48kHz)

**Lock-free communication:**
```cpp
// Audio thread (producer)
AudioFeatures features = extractFeatures(audioBuffer);
featureQueue.push(features);  // Non-blocking, wait-free

// Render thread (consumer)
AudioFeatures features;
if (featureQueue.pop(features)) {
    engine.updateAudioFeatures(features);
}
```

### Render Thread

**Responsibilities:**
- Run at fixed 60 FPS (16.67ms period)
- Consume audio features from queue
- Apply parameter changes to engine
- Render AVS effects to FBO
- Handle preset loading

**Timing:**
```cpp
void renderThreadLoop() {
    const auto framePeriod = std::chrono::milliseconds(16);  // 60 FPS
    auto nextFrame = std::chrono::steady_clock::now();

    while (running) {
        // Consume features
        processFeatureQueue();

        // Apply parameter updates
        applyParameterChanges();

        // Render frame
        renderEngine();

        // Sleep until next frame
        nextFrame += framePeriod;
        std::this_thread::sleep_until(nextFrame);
    }
}
```

**OpenGL context management:**
```cpp
// Acquire context (may block briefly)
openglContext.makeActive();

// Render to FBO
glBindFramebuffer(GL_FRAMEBUFFER, renderFBO);
engine.render();

// Release context
openglContext.release();

// Signal UI thread (new frame available)
frameReadySemaphore.post();
```

### UI Thread

**Responsibilities:**
- Display rendered frames
- Handle mouse/keyboard input
- Update UI controls (sliders, buttons)
- MIDI CC learn workflow

**Frame display (triple buffering):**
```cpp
// Render thread: Write to back buffer
frameBuffers[writeIndex] = renderEngine();
std::atomic_exchange(&readyIndex, writeIndex);
frameReadySemaphore.post();

// UI thread: Read from ready buffer
frameReadySemaphore.wait();
int index = std::atomic_load(&readyIndex);
displayFrame(frameBuffers[index]);
```

---

## Parameter System

### Stable Bank Architecture

**Core concept**: 56 VST parameters with **fixed IDs** that never change, regardless of preset.

**Implementation:**

```cpp
// Parameter definition (compile-time constant)
struct ParameterDef {
    const char* id;          // "macro.1"
    const char* label;       // "Macro 1"
    float min, max, default;
    ParamType type;          // Float, Bool, Action
    uint32_t flags;          // Momentary, Automatable, etc.
};

constexpr ParameterDef kParameterBank[] = {
    {"macro.1",  "Macro 1",  0.0f, 1.0f, 0.0f, Float, Automatable},
    {"macro.2",  "Macro 2",  0.0f, 1.0f, 0.0f, Float, Automatable},
    // ... 54 more parameters
};

constexpr size_t kNumParameters = std::size(kParameterBank);
```

### Parameter Smoothing

**Algorithm**: First-order exponential smoothing (one-pole ILP filter)

```cpp
class ParameterSmoother {
    float current;
    float target;
    float coefficient;  // Calculated from time constant

public:
    void setTarget(float newTarget) {
        target = newTarget;
    }

    float getNextValue() {
        // Exponential approach: y[n] = y[n-1] + α * (target - y[n-1])
        current += coefficient * (target - current);
        return current;
    }

    void setTimeConstant(float ms, float sampleRate) {
        // Calculate coefficient for given time constant
        // 30ms time constant = reaches 95% of target in 90ms
        coefficient = 1.0f - std::exp(-1.0f / (ms * 0.001f * sampleRate));
    }
};
```

**Usage in audio thread:**
```cpp
void processBlock(AudioBuffer& buffer) {
    const int numSamples = buffer.getNumSamples();

    for (int i = 0; i < numSamples; ++i) {
        // Get smoothed parameter value (per-sample)
        float macro1 = parameterSmoothers[0].getNextValue();

        // Apply to processing (if needed for audio-rate modulation)
        // For vis_avs, we only need control-rate (once per buffer)
    }

    // For control-rate parameters, smooth once per buffer
    float macro1 = parameterSmoothers[0].getNextValue();
    pushParameterUpdate(ParameterID::Macro1, macro1);
}
```

### Lock-free Parameter Updates

**Implementation**: Single-producer, single-consumer ring buffer

```cpp
template<typename T, size_t Size>
class SPSCRingBuffer {
    std::array<T, Size> buffer;
    std::atomic<size_t> writePos{0};
    std::atomic<size_t> readPos{0};

public:
    bool push(const T& item) {
        size_t write = writePos.load(std::memory_order_relaxed);
        size_t nextWrite = (write + 1) % Size;

        // Check if full (would catch up to read position)
        if (nextWrite == readPos.load(std::memory_order_acquire))
            return false;  // Buffer full

        buffer[write] = item;
        writePos.store(nextWrite, std::memory_order_release);
        return true;
    }

    bool pop(T& item) {
        size_t read = readPos.load(std::memory_order_relaxed);

        // Check if empty
        if (read == writePos.load(std::memory_order_acquire))
            return false;  // Buffer empty

        item = buffer[read];
        readPos.store((read + 1) % Size, std::memory_order_release);
        return true;
    }
};
```

---

## Audio Path

### Zero-Latency Passthrough

**Implementation:**

```cpp
void PluginProcessor::processBlock(AudioBuffer<float>& buffer,
                                     MidiBuffer& midiMessages) {
    ScopedNoDenormals noDenormals;

    // Input pointers
    const float* inputL = buffer.getReadPointer(0);
    const float* inputR = buffer.getReadPointer(1);

    // Output pointers
    float* outputL = buffer.getWritePointer(0);
    float* outputR = buffer.getWritePointer(1);

    const int numSamples = buffer.getNumSamples();

    // Option 1: Passthrough (in-place, zero copy if possible)
    if (inputL != outputL) {
        std::memcpy(outputL, inputL, numSamples * sizeof(float));
        std::memcpy(outputR, inputR, numSamples * sizeof(float));
    }

    // Option 2: Silent output (for analyzer-only mode)
    // buffer.clear();

    // Extract features (non-blocking)
    extractAudioFeatures(inputL, inputR, numSamples);

    // Report zero latency
    setLatencySamples(0);
}
```

### Feature Extraction

**Pipeline:**

```cpp
struct AudioFeatures {
    std::array<float, 256> spectrum;   // FFT bins (log-scale)
    std::array<float, 128> waveform;   // Time-domain samples
    float beatIntensity;               // 0..1, current beat strength
    float bassEnergy;                  // Low freq energy (20-250 Hz)
    float midEnergy;                   // Mid freq energy (250-4000 Hz)
    float highEnergy;                  // High freq energy (4000-20000 Hz)
    uint64_t timestamp;                // Sample position (for sync)
};

void extractAudioFeatures(const float* left, const float* right,
                           int numSamples) {
    // Mix to mono (simple average)
    for (int i = 0; i < numSamples; ++i) {
        monoBuffer[i] = (left[i] + right[i]) * 0.5f;
    }

    // FFT (using pre-allocated buffers, no allocations)
    fft.performFrequencyOnlyForwardTransform(monoBuffer);

    // Extract spectrum (256 log-spaced bins from FFT output)
    for (int i = 0; i < 256; ++i) {
        float freq = binToFrequency(i);
        float magnitude = getFFTMagnitude(freq);
        features.spectrum[i] = magnitude;
    }

    // Beat detection (compare current energy to recent history)
    float currentEnergy = calculateTotalEnergy(monoBuffer, numSamples);
    features.beatIntensity = beatDetector.process(currentEnergy);

    // Frequency band energy
    features.bassEnergy = calculateBandEnergy(20, 250);
    features.midEnergy = calculateBandEnergy(250, 4000);
    features.highEnergy = calculateBandEnergy(4000, 20000);

    // Timestamp (for sync with render thread)
    features.timestamp = currentSamplePosition;

    // Push to queue (non-blocking)
    featureQueue.push(features);
}
```

**Beat detection algorithm:**

```cpp
class BeatDetector {
    std::array<float, 43> energyHistory;  // ~1 second at 43 Hz
    size_t historyIndex = 0;
    float variance = 0.0f;

public:
    float process(float currentEnergy) {
        // Calculate average and variance of recent history
        float average = std::accumulate(energyHistory.begin(),
                                         energyHistory.end(), 0.0f)
                        / energyHistory.size();

        variance = 0.0f;
        for (float e : energyHistory) {
            variance += (e - average) * (e - average);
        }
        variance /= energyHistory.size();

        // Beat detected if current energy exceeds threshold
        // Threshold = average + C * variance (C = 1.5 typical)
        float threshold = average + 1.5f * std::sqrt(variance);
        float beat = (currentEnergy > threshold) ? 1.0f : 0.0f;

        // Update history
        energyHistory[historyIndex] = currentEnergy;
        historyIndex = (historyIndex + 1) % energyHistory.size();

        return beat;
    }
};
```

---

## Preset Mapping

### Control Mapping Schema

**JSON structure:**

```json
{
  "preset": {
    "controls": {
      "pages": [
        {
          "id": "main",
          "label": "Main Controls",
          "slots": [
            {
              "control": "macro.1",
              "target": "effect.superscope.n_points",
              "range": {"min": 100, "max": 4000},
              "curve": "log",
              "smoothing_ms": 40,
              "label": "Point Count",
              "color": "#FF5500"
            }
          ]
        }
      ],
      "midi_map": {
        "cc1": "macro.1",
        "cc2": "macro.2"
      }
    }
  }
}
```

### Runtime Mapping Table

**Data structure:**

```cpp
struct ControlMapping {
    std::string controlID;      // "macro.1"
    std::string targetPath;     // "effect.superscope.n_points"
    float rangeMin, rangeMax;
    CurveType curve;            // Linear, Log, Exp, S, InvS, Step
    float smoothingMs;
    std::string label;
};

class ControlMappingTable {
    // Fast lookup: Parameter ID → Mapping
    std::unordered_map<uint32_t, ControlMapping> mappings;

public:
    void setMapping(uint32_t paramID, const ControlMapping& mapping) {
        mappings[paramID] = mapping;
    }

    std::optional<ControlMapping> getMapping(uint32_t paramID) const {
        auto it = mappings.find(paramID);
        return (it != mappings.end()) ? std::make_optional(it->second)
                                       : std::nullopt;
    }

    void clear() {
        mappings.clear();
    }
};
```

### Curve Evaluation

**Implementations:**

```cpp
float applyCurve(float normalized, CurveType curve,
                 float min, float max) {
    // normalized is 0..1 from VST parameter
    float shaped;

    switch (curve) {
    case Linear:
        shaped = normalized;
        break;

    case Log:
        // Logarithmic: more precision at low end
        // Uses log base 10 for intuitive scaling
        shaped = (std::log10(normalized * 9.0f + 1.0f));
        break;

    case Exp:
        // Exponential: more precision at high end
        shaped = (std::pow(10.0f, normalized) - 1.0f) / 9.0f;
        break;

    case S:
        // S-curve (smoothstep): slow-fast-slow
        shaped = normalized * normalized * (3.0f - 2.0f * normalized);
        break;

    case InvS:
        // Inverse S-curve (inverse smoothstep): fast-slow-fast
        shaped = (normalized < 0.5f)
                 ? 0.5f * std::pow(2.0f * normalized, 2.0f)
                 : 1.0f - 0.5f * std::pow(2.0f - 2.0f * normalized, 2.0f);
        break;

    case Step:
        // Quantized steps (specified in mapping)
        int steps = mapping.quantizeSteps;
        shaped = std::floor(normalized * steps) / steps;
        break;
    }

    // Apply range
    return min + shaped * (max - min);
}
```

### Preset Load Workflow

**Sequence:**

```cpp
void loadPreset(const std::string& presetPath) {
    // 1. Parse preset JSON
    auto preset = PresetLoader::load(presetPath);

    // 2. Extract controls section
    auto controls = preset.controls;

    // 3. Update mapping table (on render thread, not audio thread)
    renderThread.enqueue([this, controls]() {
        mappingTable.clear();

        for (const auto& slot : controls.slots) {
            uint32_t paramID = getParameterID(slot.control);
            mappingTable.setMapping(paramID, slot);
        }

        // 4. Load engine preset (graph, effects, etc.)
        engine.loadPreset(preset);

        // 5. Ramp parameters to avoid pops
        rampParametersToCurrentValues(60ms);
    });
}
```

---

## Render Pipeline

### OpenGL Context Setup (JUCE)

```cpp
class PluginEditor : public juce::AudioProcessorEditor,
                      private juce::OpenGLRenderer {
public:
    PluginEditor(PluginProcessor& p) : AudioProcessorEditor(p) {
        // Attach OpenGL context to this component
        openGLContext.setRenderer(this);
        openGLContext.attachTo(*this);
        openGLContext.setContinuousRepainting(true);  // 60 FPS
    }

    ~PluginEditor() override {
        openGLContext.detach();
    }

private:
    // OpenGLRenderer callbacks
    void newOpenGLContextCreated() override {
        // Initialize engine OpenGL resources
        engine.initGL();
    }

    void renderOpenGL() override {
        // Called at 60 FPS by JUCE
        engine.render();
    }

    void openGLContextClosing() override {
        // Cleanup engine OpenGL resources
        engine.cleanupGL();
    }

    juce::OpenGLContext openGLContext;
    AVSEngine engine;
};
```

### Frame Synchronization

**Challenge**: Audio thread (variable rate), render thread (60 FPS), UI thread (vsync)

**Solution**: Decouple rates with lock-free queues

```cpp
// Audio thread: 43-189 Hz (depends on buffer size)
//   pushes features at buffer rate
featureQueue.push(features);  // ~100 Hz typical

// Render thread: Fixed 60 Hz
//   consumes all available features each frame
while (featureQueue.pop(features)) {
    engine.updateFeatures(features);  // Takes most recent
}
engine.render();  // Always at 60 FPS

// UI thread: Vsync rate (60 Hz typical, but can vary)
//   displays most recently rendered frame
displayFrame(engine.getLastFrame());
```

---

## MIDI Processing

### CC Learn Implementation

```cpp
class MIDICCLearn {
    enum class State { Idle, Learning };

    State state = State::Idle;
    uint32_t learningParamID = 0;

public:
    void startLearning(uint32_t paramID) {
        state = State::Learning;
        learningParamID = paramID;
    }

    void processMIDI(const MidiMessage& msg) {
        if (state == State::Learning && msg.isController()) {
            int ccNumber = msg.getControllerNumber();

            // Create mapping
            midiCCMap.set(ccNumber, learningParamID);

            // Exit learn mode
            state = State::Idle;

            // Notify UI
            listeners.call(&Listener::ccMappingCreated, ccNumber, learningParamID);
        }
    }
};
```

### Transport Sync

```cpp
void processBlock(AudioBuffer& buffer, MidiBuffer& midi) {
    // Get transport info from host
    AudioPlayHead* playHead = getPlayHead();
    if (playHead != nullptr) {
        AudioPlayHead::CurrentPositionInfo posInfo;
        playHead->getCurrentPosition(posInfo);

        // Extract relevant info
        double bpm = posInfo.bpm;
        double ppqPosition = posInfo.ppqPosition;  // Quarter notes
        bool isPlaying = posInfo.isPlaying;

        // Push to render thread (lock-free)
        transportQueue.push({bpm, ppqPosition, isPlaying});
    }

    // ... rest of processing
}
```

### Scene Quantization

```cpp
class SceneQuantizer {
    double lastPPQ = 0.0;
    double quantizeInterval = 1.0;  // 1 quarter note

    struct PendingTrigger {
        int sceneID;
        double triggerPPQ;
    };
    std::vector<PendingTrigger> pending;

public:
    void setQuantization(double bars) {
        quantizeInterval = bars * 4.0;  // Bars to quarter notes
    }

    void triggerScene(int sceneID, double currentPPQ) {
        // Calculate next quantize point
        double nextPPQ = std::ceil(currentPPQ / quantizeInterval)
                         * quantizeInterval;

        pending.push_back({sceneID, nextPPQ});
    }

    void update(double currentPPQ) {
        // Check if any pending triggers should fire
        auto it = std::remove_if(pending.begin(), pending.end(),
            [currentPPQ, this](const PendingTrigger& t) {
                if (currentPPQ >= t.triggerPPQ) {
                    engine.activateScene(t.sceneID);
                    return true;  // Remove from pending
                }
                return false;
            });

        pending.erase(it, pending.end());
    }
};
```

---

## State Management

### Plugin State Structure

```cpp
struct PluginState {
    // Current preset
    std::string presetPath;
    PresetData presetData;

    // Parameter values (56 floats)
    std::array<float, 56> parameters;

    // Control mapping
    ControlMappingTable mappings;

    // MIDI CC map
    std::unordered_map<int, uint32_t> ccToParam;  // CC# → ParamID

    // Editor state
    int windowWidth, windowHeight;
    bool editorOpen;
};
```

### Serialization

```cpp
void PluginProcessor::getStateInformation(MemoryBlock& destData) {
    // Use JSON for human-readable state
    json state;

    // Serialize parameters
    state["parameters"] = std::vector<float>(
        currentParameters.begin(),
        currentParameters.end()
    );

    // Serialize current preset
    state["preset"] = currentPresetData.toJSON();

    // Serialize MIDI CC mappings
    for (const auto& [cc, paramID] : midiCCMap) {
        state["midi_cc"][std::to_string(cc)] = paramID;
    }

    // Convert to binary
    std::string jsonString = state.dump();
    destData.append(jsonString.data(), jsonString.size());
}

void PluginProcessor::setStateInformation(const void* data,
                                            int sizeInBytes) {
    // Parse JSON state
    std::string jsonString(static_cast<const char*>(data), sizeInBytes);
    json state = json::parse(jsonString);

    // Restore parameters
    auto params = state["parameters"].get<std::vector<float>>();
    for (size_t i = 0; i < params.size(); ++i) {
        setParameter(i, params[i]);
    }

    // Restore preset
    loadPresetFromJSON(state["preset"]);

    // Restore MIDI CC mappings
    for (const auto& [ccStr, paramID] : state["midi_cc"].items()) {
        int cc = std::stoi(ccStr);
        midiCCMap[cc] = paramID;
    }
}
```

---

## Performance Optimization

### Profiling Strategy

**Tools:**
- Linux: `perf`, `valgrind --tool=callgrind`
- macOS: Instruments (Time Profiler)
- Windows: Visual Studio Profiler, Intel VTune

**Key metrics:**
- processBlock() time: Must be <50% of buffer time
- Render frame time: Must be <16.67ms (60 FPS)
- Memory allocations in processBlock(): Must be 0

**Example profiling:**

```bash
# Linux: Profile audio thread
perf record -F 99 -p $(pgrep reaper) --call-graph dwarf
perf report

# Check for allocations in processBlock()
valgrind --tool=massif ./host_with_plugin
ms_print massif.out.12345
```

### Optimization Techniques

**1. SIMD for audio processing:**

```cpp
// Scalar version
for (int i = 0; i < numSamples; ++i) {
    mono[i] = (left[i] + right[i]) * 0.5f;
}

// SSE version (4x faster)
for (int i = 0; i < numSamples; i += 4) {
    __m128 l = _mm_loadu_ps(&left[i]);
    __m128 r = _mm_loadu_ps(&right[i]);
    __m128 sum = _mm_add_ps(l, r);
    __m128 mono = _mm_mul_ps(sum, _mm_set1_ps(0.5f));
    _mm_storeu_ps(&monoBuffer[i], mono);
}
```

**2. Avoid branching in inner loops:**

```cpp
// Bad: Branch in tight loop
for (int i = 0; i < 256; ++i) {
    if (spectrum[i] > threshold) {
        output[i] = spectrum[i];
    } else {
        output[i] = 0.0f;
    }
}

// Good: Branchless
for (int i = 0; i < 256; ++i) {
    float mask = (spectrum[i] > threshold) ? 1.0f : 0.0f;
    output[i] = spectrum[i] * mask;
}
```

**3. Cache-friendly data structures:**

```cpp
// Bad: Array of structs (poor cache locality)
struct Particle {
    float x, y, z;
    float vx, vy, vz;
    uint32_t color;
};
std::vector<Particle> particles(10000);

// Good: Struct of arrays (better cache locality)
struct ParticleSystem {
    std::vector<float> x, y, z;
    std::vector<float> vx, vy, vz;
    std::vector<uint32_t> colors;
};
```

---

## Error Handling

### Strategy

**Audio thread**: Never throw, never crash. Log errors and continue.

```cpp
void processBlock(AudioBuffer& buffer, MidiBuffer& midi) {
    try {
        // ... processing
    } catch (const std::exception& e) {
        // Log error (lock-free logging)
        errorLog.push(e.what());

        // Ensure output is valid (silence)
        buffer.clear();
    }
}
```

**Render thread**: Tolerate occasional frame drops, never crash.

```cpp
void renderFrame() {
    try {
        engine.render();
    } catch (const std::exception& e) {
        // Log error
        std::cerr << "Render error: " << e.what() << std::endl;

        // Fallback: Clear screen
        glClear(GL_COLOR_BUFFER_BIT);
    }
}
```

**UI thread**: Show errors to user.

```cpp
void loadPreset(const std::string& path) {
    try {
        presetLoader.load(path);
    } catch (const PresetLoadError& e) {
        // Show error dialog
        AlertWindow::showMessageBoxAsync(
            AlertWindow::WarningIcon,
            "Preset Load Error",
            e.what()
        );
    }
}
```

---

## Platform-Specific Considerations

### Windows

- **Thread priority**: Use `SetThreadPriority(THREAD_PRIORITY_TIME_CRITICAL)` for audio thread
- **WASAPI**: Host may use exclusive mode (avoid DirectSound assumptions)
- **Code signing**: Authenticode required to avoid SmartScreen warnings

### macOS

- **Core Audio**: Extremely strict real-time requirements (no syscalls in audio thread)
- **Code signing**: Required for Gatekeeper. Use `codesign` and notarization.
- **High DPI**: Handle Retina displays (2x scale factor)

### Linux

- **Real-time kernel**: Recommend `PREEMPT_RT` patch for best audio performance
- **OpenGL**: Test with both X11 and Wayland
- **VST3 path**: `~/.vst3/` or `/usr/lib/vst3/`

---

## Summary

This architecture provides:
- **Real-time safety**: Lock-free communication, no allocations in audio thread
- **Performance**: SIMD, cache-friendly data, <5% CPU usage
- **Stability**: Error handling, defensive programming, comprehensive testing
- **Flexibility**: Modular design, extensible parameter system

Next: Proceed to implementation (Task 25: Framework Integration)
