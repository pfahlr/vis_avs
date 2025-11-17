# AVS Pipewire Audio Backend

Modern Linux audio input backend using Pipewire for low-latency audio capture.

## Status

**Implementation**: Stub with complete API (Job #20 - IN_PROGRESS)

The API and structure are fully defined, but actual Pipewire integration requires `libpipewire-0.3-dev`.

## Overview

Pipewire is the successor to PulseAudio and JACK, providing:
- Lower latency (<10ms target)
- Better device management
- Hot-swapping support
- Professional audio features
- Modern Linux audio architecture

## Building

### With Pipewire support (Linux):

```bash
# Install Pipewire development library
sudo apt install libpipewire-0.3-dev  # Debian/Ubuntu
sudo dnf install pipewire-devel       # Fedora
sudo pacman -S pipewire               # Arch

# Build with Pipewire enabled
cmake .. -DAVS_ENABLE_PIPEWIRE=ON
cmake --build .
```

### Without Pipewire (stub only):

```bash
# Builds as stub library on systems without Pipewire
cmake ..
cmake --build .
```

The stub provides the full API but all methods return empty/false values.

## Implementation Requirements

To complete the Pipewire backend, implement the following in `src/pipewire_input.cpp`:

### 1. Initialization (`start()`)

```cpp
bool PipewireInput::start() {
  // Initialize Pipewire
  pw_init(nullptr, nullptr);

  // Create context
  impl_->loop = pw_thread_loop_new("avs-audio", nullptr);
  impl_->context = pw_context_new(
      pw_thread_loop_get_loop(impl_->loop), nullptr, 0);

  // Create stream for audio capture
  impl_->stream = pw_stream_new_simple(
      pw_thread_loop_get_loop(impl_->loop),
      "AVS Audio Capture",
      pw_properties_new(
          PW_KEY_MEDIA_TYPE, "Audio",
          PW_KEY_MEDIA_CATEGORY, "Capture",
          PW_KEY_MEDIA_ROLE, "Music",
          nullptr),
      &impl_->stream_events,
      impl_.get());

  // Connect to audio source
  pw_stream_connect(impl_->stream,
                    PW_DIRECTION_INPUT,
                    PW_ID_ANY,
                    flags,
                    params, n_params);

  // Start thread loop
  pw_thread_loop_start(impl_->loop);
  return true;
}
```

### 2. Audio Capture (`read()`)

```cpp
std::size_t PipewireInput::read(float* buffer, std::size_t frameCount) {
  pw_thread_loop_lock(impl_->loop);

  // Dequeue buffer from stream
  struct pw_buffer* b = pw_stream_dequeue_buffer(impl_->stream);
  if (!b) {
    pw_thread_loop_unlock(impl_->loop);
    return 0;
  }

  // Copy audio data
  struct spa_buffer* buf = b->buffer;
  struct spa_data* d = &buf->datas[0];
  float* samples = (float*)d->data;
  size_t n_samples = d->chunk->size / sizeof(float);

  std::memcpy(buffer, samples, std::min(n_samples, frameCount * impl_->channels) * sizeof(float));

  // Return buffer to stream
  pw_stream_queue_buffer(impl_->stream, b);
  pw_thread_loop_unlock(impl_->loop);

  return frameCount;
}
```

### 3. Device Enumeration (`enumerateDevices()`)

```cpp
std::vector<DeviceInfo> PipewireInput::enumerateDevices() {
  std::vector<DeviceInfo> devices;

  // Get registry
  struct pw_registry* registry = pw_core_get_registry(core, ...);

  // Listen for nodes
  spa_hook registry_listener;
  pw_registry_add_listener(registry, &registry_listener,
                           &registry_events, nullptr);

  // Collect Audio/Source nodes
  // Each node provides: id, name, sample rate, channels

  return devices;
}
```

### 4. Runtime Detection (`isAvailable()`)

```cpp
bool PipewireInput::isAvailable() {
  #ifdef AVS_HAS_PIPEWIRE
    // Check if Pipewire daemon is running
    const char* runtime_dir = getenv("PIPEWIRE_RUNTIME_DIR");
    if (!runtime_dir) {
      runtime_dir = getenv("XDG_RUNTIME_DIR");
    }

    // Try to connect to Pipewire
    struct pw_main_loop* loop = pw_main_loop_new(nullptr);
    struct pw_context* context = pw_context_new(pw_main_loop_get_loop(loop), nullptr, 0);
    struct pw_core* core = pw_context_connect(context, nullptr, 0);

    bool available = (core != nullptr);

    // Cleanup
    if (core) pw_core_disconnect(core);
    if (context) pw_context_destroy(context);
    if (loop) pw_main_loop_destroy(loop);

    return available;
  #else
    return false;
  #endif
}
```

## Usage Example

```cpp
#include <avs/audio/pipewire_input.h>

// Check if Pipewire is available
if (avs::audio::PipewireInput::isAvailable()) {
  // Create Pipewire input (48kHz stereo)
  avs::audio::PipewireInput input(48000, 2);

  // Start capture
  if (input.start()) {
    // Read audio samples
    std::vector<float> buffer(1024 * 2);  // 1024 frames, 2 channels
    size_t framesRead = input.read(buffer.data(), 1024);

    // Process audio...

    // Stop when done
    input.stop();
  }
} else {
  // Fall back to PortAudio
  std::fprintf(stderr, "Pipewire not available, using PortAudio\n");
}
```

## Integration with avs-player

Future enhancement (not yet implemented):

```cpp
// Command-line option
--audio-backend=pipewire|portaudio

// Implementation in main.cpp
std::string backend = getAudioBackend(args);
if (backend == "pipewire" && PipewireInput::isAvailable()) {
  audioInput = std::make_unique<PipewireInput>(sampleRate, channels);
} else {
  audioInput = std::make_unique<PortAudioInput>(sampleRate, channels);
}
```

## Testing

### Manual Testing

```bash
# Check if Pipewire is running
systemctl --user status pipewire

# List audio sources
pw-cli ls Node | grep -A5 "Audio/Source"

# Monitor audio capture
pw-cat --record test.wav
```

### Automated Testing

Unit tests with mocked Pipewire API (to be implemented):
- `tests/pipewire_mock_test.cpp`
- Verify device enumeration
- Test audio buffer handling
- Validate error conditions

## References

- [Pipewire Documentation](https://docs.pipewire.org/)
- [Pipewire Tutorial](https://docs.pipewire.org/page_tutorial.html)
- AVS2K25 Pipewire implementation (reference)
- [Linux Audio Architecture](https://wiki.archlinux.org/title/PipeWire)

## Status Summary

**Complete**:
- ✅ API design
- ✅ Header file
- ✅ Stub implementation
- ✅ CMake integration with optional Pipewire detection
- ✅ Build system integration

**TODO**:
- ⏳ Pipewire stream initialization
- ⏳ Audio buffer management
- ⏳ Device enumeration
- ⏳ Runtime availability detection
- ⏳ Integration with avs-player
- ⏳ Automated tests

**Estimated Completion**: 4-6 hours for experienced developer with Pipewire knowledge
