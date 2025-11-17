#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace avs::audio {

/**
 * @brief Pipewire audio input backend for modern Linux systems.
 *
 * Pipewire is the successor to PulseAudio/JACK and provides:
 * - Lower latency audio capture
 * - Better device management
 * - Hot-swapping support
 * - Pro-audio features
 *
 * This backend is Linux-only and requires libpipewire-0.3.
 */
class PipewireInput {
 public:
  struct DeviceInfo {
    std::string id;
    std::string name;
    std::uint32_t sampleRate;
    std::uint32_t channels;
    bool isDefault;
  };

  /**
   * @brief Initialize Pipewire audio capture.
   *
   * @param sampleRate Desired sample rate (e.g., 44100, 48000)
   * @param channels Number of audio channels (1 = mono, 2 = stereo)
   * @param deviceId Optional specific device ID (empty = use default)
   */
  PipewireInput(std::uint32_t sampleRate, std::uint32_t channels,
                const std::string& deviceId = "");

  ~PipewireInput();

  /**
   * @brief Start audio capture.
   *
   * @return true if started successfully, false otherwise
   */
  bool start();

  /**
   * @brief Stop audio capture.
   */
  void stop();

  /**
   * @brief Check if audio capture is active.
   */
  bool isActive() const;

  /**
   * @brief Read captured audio samples.
   *
   * @param buffer Output buffer for audio samples
   * @param frameCount Number of frames to read
   * @return Number of frames actually read
   */
  std::size_t read(float* buffer, std::size_t frameCount);

  /**
   * @brief Enumerate available Pipewire audio sources.
   *
   * @return List of available audio capture devices
   */
  static std::vector<DeviceInfo> enumerateDevices();

  /**
   * @brief Check if Pipewire is available on this system.
   *
   * @return true if Pipewire runtime is available, false otherwise
   */
  static bool isAvailable();

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace avs::audio
