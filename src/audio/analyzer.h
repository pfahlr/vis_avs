#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <vector>

namespace avs::audio {

struct Analysis {
  static constexpr std::size_t kFftSize = 1024;
  static constexpr std::size_t kSpectrumSize = kFftSize / 2;
  static constexpr std::size_t kWaveformSize = 576;

  std::array<float, kSpectrumSize> spectrum{};
  std::array<float, kWaveformSize> waveform{};
  bool beat = false;
  float bpm = 0.0f;
  float bass = 0.0f;
  float mid = 0.0f;
  float treb = 0.0f;
  float confidence = 0.0f;
};

class Analyzer {
 public:
  Analyzer(int sampleRate, int channels);
  ~Analyzer();

  Analyzer(const Analyzer&) = delete;
  Analyzer& operator=(const Analyzer&) = delete;
  Analyzer(Analyzer&&) noexcept;
  Analyzer& operator=(Analyzer&&) noexcept;

  int sampleRate() const { return sampleRate_; }
  int channels() const { return channels_; }

  void reset();

  const Analysis& process(const float* interleavedSamples, std::size_t frameCount);

  void setDampingEnabled(bool enabled) { dampingEnabled_ = enabled; }
  bool dampingEnabled() const { return dampingEnabled_; }

 private:
  void updateSpectrum();
  void updateWaveform();
  void updateBands();
  void updateBeat();

  static float hzForBin(std::size_t bin, int sampleRate);

  int sampleRate_ = 0;
  int channels_ = 0;
  bool dampingEnabled_ = true;

  std::array<float, Analysis::kFftSize> monoWindowed_{};
  std::array<float, Analysis::kSpectrumSize> magnitude_{};

  std::vector<float> window_;
  std::deque<float> energyHistory_;
  float lastEnergy_ = 0.0f;
  float lastBeatTimeSeconds_ = 0.0f;
  float accumulatedTime_ = 0.0f;
  int framesProcessed_ = 0;
  float bpmSmoothing_ = 0.0f;
  float confidenceSmoothing_ = 0.0f;

  Analysis analysis_;

  struct FFTPlan;
  FFTPlan* fft_ = nullptr;
};

}  // namespace avs::audio
