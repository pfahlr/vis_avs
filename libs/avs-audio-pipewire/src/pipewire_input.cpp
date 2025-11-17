#include <avs/audio/pipewire_input.h>

#include <cstring>

namespace avs::audio {

// PIMPL implementation (stub - requires libpipewire-0.3 for full implementation)
struct PipewireInput::Impl {
  std::uint32_t sampleRate;
  std::uint32_t channels;
  std::string deviceId;
  bool active = false;

  Impl(std::uint32_t rate, std::uint32_t ch, const std::string& dev)
      : sampleRate(rate), channels(ch), deviceId(dev) {}
};

PipewireInput::PipewireInput(std::uint32_t sampleRate, std::uint32_t channels,
                             const std::string& deviceId)
    : impl_(std::make_unique<Impl>(sampleRate, channels, deviceId)) {}

PipewireInput::~PipewireInput() {
  stop();
}

bool PipewireInput::start() {
  // TODO: Initialize Pipewire context and stream
  // pw_init()
  // pw_context_new()
  // pw_stream_new() with audio/capture
  // pw_stream_connect()
  impl_->active = false;  // Not implemented yet
  return impl_->active;
}

void PipewireInput::stop() {
  // TODO: Cleanup Pipewire resources
  // pw_stream_disconnect()
  // pw_stream_destroy()
  // pw_context_destroy()
  impl_->active = false;
}

bool PipewireInput::isActive() const {
  return impl_->active;
}

std::size_t PipewireInput::read(float* buffer, std::size_t frameCount) {
  if (!impl_->active) {
    // Fill with silence when not active
    std::memset(buffer, 0, frameCount * impl_->channels * sizeof(float));
    return frameCount;
  }

  // TODO: Read from Pipewire stream buffer
  // pw_stream_dequeue_buffer()
  // Copy audio data to output buffer
  // pw_stream_queue_buffer()

  // Stub: return silence
  std::memset(buffer, 0, frameCount * impl_->channels * sizeof(float));
  return frameCount;
}

std::vector<PipewireInput::DeviceInfo> PipewireInput::enumerateDevices() {
  std::vector<DeviceInfo> devices;

  // TODO: Enumerate Pipewire sources using registry
  // pw_core_get_registry()
  // Listen for PW_TYPE_INTERFACE_Node with media.class=Audio/Source
  // Collect device info (id, name, sample rate, channels)

  // Stub: return empty list
  return devices;
}

bool PipewireInput::isAvailable() {
  // TODO: Check if libpipewire-0.3 is available at runtime
  // dlopen("libpipewire-0.3.so") or check for pw_init symbol
  return false;  // Not implemented yet
}

}  // namespace avs::audio
