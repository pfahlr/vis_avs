#include "AudioEngine.hpp"

#include <portaudio.h>

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace avs::audio {
namespace {

class PortAudioSession {
 public:
  PortAudioSession() {
    PaError err = Pa_Initialize();
    if (err != paNoError) {
      throw std::runtime_error(std::string("Failed to initialize PortAudio: ") +
                               Pa_GetErrorText(err));
    }
  }
  ~PortAudioSession() { Pa_Terminate(); }
};

PortAudioSession& globalSession() {
  static PortAudioSession session;
  return session;
}

std::string makePaErrorMessage(const std::string& prefix, PaError err) {
  std::ostringstream oss;
  oss << prefix << ": " << Pa_GetErrorText(err);
  return oss.str();
}

}  // namespace

struct AudioEngine::Impl {
  Impl() { (void)globalSession(); }
};

struct AudioEngine::InputStream::Impl {
  PaStream* stream = nullptr;
  InputCallback callback;
  int channelCount = 0;
};

namespace {

int paStreamCallback(const void* input, void*, unsigned long frameCount,
                     const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags,
                     void* userData) {
  auto* impl = static_cast<AudioEngine::InputStream::Impl*>(userData);
  if (!impl) {
    return paAbort;
  }

  const float* samples = static_cast<const float*>(input);
  thread_local std::vector<float> silence;
  if (!samples) {
    silence.assign(frameCount * static_cast<unsigned long>(std::max(1, impl->channelCount)), 0.0f);
    samples = silence.data();
  }

  if (impl->callback) {
    double streamTime = timeInfo ? timeInfo->currentTime : 0.0;
    impl->callback(samples, frameCount, impl->channelCount, streamTime);
  }
  return paContinue;
}

bool sampleRateMatches(const DeviceInfo& device, double preferred) {
  if (preferred <= 0.0) {
    return true;
  }
  if (device.defaultSampleRate <= 0.0) {
    return false;
  }
  return std::fabs(device.defaultSampleRate - preferred) <= 1.0;
}

}  // namespace

AudioEngine::AudioEngine() : impl_(std::make_unique<Impl>()) {}
AudioEngine::~AudioEngine() = default;
AudioEngine::AudioEngine(AudioEngine&&) noexcept = default;
AudioEngine& AudioEngine::operator=(AudioEngine&&) noexcept = default;

std::vector<DeviceInfo> AudioEngine::listInputDevices() const {
  (void)globalSession();
  PaDeviceIndex count = Pa_GetDeviceCount();
  if (count < 0) {
    throw std::runtime_error(makePaErrorMessage("Failed to enumerate audio devices", count));
  }

  PaDeviceIndex defaultInput = Pa_GetDefaultInputDevice();
  PaDeviceIndex defaultOutput = Pa_GetDefaultOutputDevice();

  std::vector<DeviceInfo> devices;
  devices.reserve(static_cast<size_t>(count));
  for (PaDeviceIndex i = 0; i < count; ++i) {
    const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
    if (!info) {
      continue;
    }
    DeviceInfo device;
    device.index = static_cast<int>(i);
    device.name = info->name ? info->name : "Unknown";
    device.maxInputChannels = info->maxInputChannels;
    device.maxOutputChannels = info->maxOutputChannels;
    device.defaultSampleRate = info->defaultSampleRate;
    device.isDefaultInput = (i == defaultInput);
    device.isDefaultOutput = (i == defaultOutput);
    devices.push_back(std::move(device));
  }
  return devices;
}

AudioEngine::InputStream::InputStream() = default;
AudioEngine::InputStream::~InputStream() {
  if (!impl_) {
    return;
  }
  if (impl_->stream) {
    Pa_StopStream(impl_->stream);
    Pa_CloseStream(impl_->stream);
  }
}
AudioEngine::InputStream::InputStream(InputStream&&) noexcept = default;
AudioEngine::InputStream& AudioEngine::InputStream::operator=(InputStream&&) noexcept = default;

bool AudioEngine::InputStream::isActive() const {
  if (!impl_ || !impl_->stream) {
    return false;
  }
  return Pa_IsStreamActive(impl_->stream) == 1;
}

AudioEngine::InputStream::InputStream(std::shared_ptr<Impl> impl) : impl_(std::move(impl)) {}

AudioEngine::InputStream AudioEngine::openInputStream(const DeviceInfo& device, double sampleRate,
                                                      unsigned long framesPerBuffer,
                                                      InputCallback callback) const {
  if (!device.isInputCapable()) {
    throw std::runtime_error("Selected device \"" + device.name + "\" has no input channels.");
  }

  const PaDeviceInfo* info = Pa_GetDeviceInfo(static_cast<PaDeviceIndex>(device.index));
  if (!info) {
    throw std::runtime_error("Failed to query PortAudio device info for index " +
                             std::to_string(device.index));
  }

  PaStreamParameters input{};
  input.device = static_cast<PaDeviceIndex>(device.index);
  input.channelCount = std::max(1, std::min(device.maxInputChannels, 2));
  input.sampleFormat = paFloat32;
  input.suggestedLatency = info->defaultLowInputLatency;
  input.hostApiSpecificStreamInfo = nullptr;

  double rate = sampleRate > 0.0 ? sampleRate : info->defaultSampleRate;
  if (rate <= 0.0) {
    rate = 48000.0;
  }

  PaError supported = Pa_IsFormatSupported(&input, nullptr, rate);
  if (supported != paFormatIsSupported) {
    std::ostringstream oss;
    oss << "Device \"" << device.name << "\" does not support " << input.channelCount
        << " channel(s) at " << rate << " Hz (" << Pa_GetErrorText(supported) << ")";
    throw std::runtime_error(oss.str());
  }

  auto impl = std::make_shared<InputStream::Impl>();
  impl->callback = std::move(callback);
  impl->channelCount = input.channelCount;

  PaStream* stream = nullptr;
  PaError err = Pa_OpenStream(&stream, &input, nullptr, rate, framesPerBuffer, paClipOff,
                              &paStreamCallback, impl.get());
  if (err != paNoError) {
    throw std::runtime_error(makePaErrorMessage("Failed to open input stream", err));
  }

  impl->stream = stream;
  err = Pa_StartStream(stream);
  if (err != paNoError) {
    Pa_CloseStream(stream);
    throw std::runtime_error(makePaErrorMessage("Failed to start input stream", err));
  }

  return InputStream(std::move(impl));
}

DeviceInfo selectInputDevice(const std::vector<DeviceInfo>& devices,
                             std::optional<DeviceSpecifier> requested,
                             double preferredSampleRate) {
  if (devices.empty()) {
    throw std::runtime_error("No audio capture devices are available.");
  }

  auto findByIndex = [&](int index) -> const DeviceInfo* {
    for (const auto& device : devices) {
      if (device.index == index) {
        return &device;
      }
    }
    return nullptr;
  };

  auto findByName = [&](const std::string& name) -> const DeviceInfo* {
    for (const auto& device : devices) {
      if (device.name == name) {
        return &device;
      }
    }
    return nullptr;
  };

  if (requested.has_value()) {
    if (const auto* index = std::get_if<int>(&*requested)) {
      const DeviceInfo* device = findByIndex(*index);
      if (!device) {
        throw std::runtime_error("Input device index " + std::to_string(*index) +
                                 " was not found.");
      }
      if (!device->isInputCapable()) {
        throw std::runtime_error("Input device index " + std::to_string(*index) +
                                 " has no input channels.");
      }
      return *device;
    }
    if (const auto* name = std::get_if<std::string>(&*requested)) {
      const DeviceInfo* device = findByName(*name);
      if (!device) {
        throw std::runtime_error("Input device \"" + *name + "\" was not found.");
      }
      if (!device->isInputCapable()) {
        throw std::runtime_error("Input device \"" + *name + "\" has no input channels.");
      }
      return *device;
    }
  }

  for (const auto& device : devices) {
    if (device.isFullDuplex() && device.isInputCapable() &&
        sampleRateMatches(device, preferredSampleRate)) {
      return device;
    }
  }

  for (const auto& device : devices) {
    if (device.isInputCapable()) {
      return device;
    }
  }

  throw std::runtime_error("No capture-capable devices are available.");
}

}  // namespace avs::audio
