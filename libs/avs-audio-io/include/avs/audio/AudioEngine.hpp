#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

#include "DeviceInfo.hpp"

namespace avs::audio {

using DeviceSpecifier = std::variant<int, std::string>;

DeviceInfo selectInputDevice(const std::vector<DeviceInfo>& devices,
                             std::optional<DeviceSpecifier> requested,
                             double preferredSampleRate);

class AudioEngine {
 public:
  using InputCallback = std::function<void(const float* samples, unsigned long frameCount,
                                           int channelCount, double streamTime)>;

  AudioEngine();
  ~AudioEngine();

  AudioEngine(const AudioEngine&) = delete;
  AudioEngine& operator=(const AudioEngine&) = delete;
  AudioEngine(AudioEngine&&) noexcept;
  AudioEngine& operator=(AudioEngine&&) noexcept;

  std::vector<DeviceInfo> listInputDevices() const;

  class InputStream {
   public:
    InputStream();
    ~InputStream();

    InputStream(InputStream&&) noexcept;
    InputStream& operator=(InputStream&&) noexcept;

    InputStream(const InputStream&) = delete;
    InputStream& operator=(const InputStream&) = delete;

    bool isActive() const;

    struct Impl;

   private:
    std::shared_ptr<Impl> impl_;

    explicit InputStream(std::shared_ptr<Impl> impl);
    friend class AudioEngine;
  };

  InputStream openInputStream(const DeviceInfo& device, double sampleRate,
                              unsigned long framesPerBuffer, InputCallback callback) const;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace avs::audio
