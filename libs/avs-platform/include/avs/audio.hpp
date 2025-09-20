#pragma once

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace avs {

struct AudioState {
  static constexpr size_t kLegacyVisSamples = 576;
  using LegacyBuffer = std::array<float, kLegacyVisSamples>;

  float rms = 0.0f;                             // 0..1
  std::array<float, 3> bands{{0.f, 0.f, 0.f}};  // bass, mid, treble smoothed
  std::vector<float> spectrum;                  // N/2 magnitudes [0,1]
  std::array<LegacyBuffer, 2> spectrumLegacy{}; // legacy 576-bin FFT view per channel
  std::array<LegacyBuffer, 2> oscilloscope{};   // legacy 576-sample oscilloscope per channel
  double timeSeconds = 0.0;                     // audio clock
  int sampleRate = 0;                           // rate of data stored in spectrum
  int inputSampleRate = 0;                      // physical capture device rate
  int channels = 0;                             // channel count used for analysis
};

struct AudioInputConfig {
  int engineSampleRate = 48000;
  int engineChannels = 2;
  std::optional<int> requestedSampleRate;
  std::optional<int> requestedChannels;
  std::optional<std::string> requestedDevice;
};

class AudioInput {
 public:
  explicit AudioInput(const AudioInputConfig& config = AudioInputConfig{});
  ~AudioInput();

  bool ok() const { return ok_; }

  // Poll produces a snapshot computed from internal ring buffer.
  AudioState poll();

 private:
  struct Impl;
  Impl* impl_ = nullptr;
  bool ok_ = false;
};

}  // namespace avs
