#include <avs/audio/analyzer.h>

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

extern "C" {
#include "kissfft/kiss_fft.h"
#include "kissfft/kiss_fftr.h"
}

namespace avs::audio {

namespace {
constexpr float kBeatThreshold = 1.35f;
constexpr std::size_t kEnergyWindow = 43;  // ~1s at 1024 hop / 44100 Hz
constexpr float kMinEnergy = 1e-6f;
constexpr float kMaxConfidence = 4.0f;
constexpr float kDampingFactor = 0.6f;
constexpr float kNoDampingFactor = 0.0f;

std::vector<float> makeHannWindow(std::size_t size) {
  constexpr double pi = 3.14159265358979323846;
  std::vector<float> window(size);
  const double factor = 2.0 * pi / static_cast<double>(size);
  for (std::size_t i = 0; i < size; ++i) {
    window[i] = static_cast<float>(0.5 * (1.0 - std::cos(factor * static_cast<double>(i))));
  }
  return window;
}

float lerp(float a, float b, float t) { return a + (b - a) * t; }

}  // namespace

struct Analyzer::FFTPlan {
  explicit FFTPlan(std::size_t n) {
    cfg = kiss_fftr_alloc(static_cast<int>(n), 0, nullptr, nullptr);
    if (!cfg) {
      throw std::bad_alloc();
    }
    freq.resize(n / 2 + 1);
  }

  ~FFTPlan() { kiss_fft_free(cfg); }

  kiss_fftr_cfg cfg = nullptr;
  std::vector<kiss_fft_cpx> freq;
};

Analyzer::Analyzer(int sampleRate, int channels)
    : sampleRate_(sampleRate),
      channels_(std::max(1, channels)),
      window_(makeHannWindow(Analysis::kFftSize)),
      analysis_{} {
  fft_ = new FFTPlan(Analysis::kFftSize);
  reset();
}

Analyzer::~Analyzer() { delete fft_; }

Analyzer::Analyzer(Analyzer&& other) noexcept { *this = std::move(other); }

Analyzer& Analyzer::operator=(Analyzer&& other) noexcept {
  if (this == &other) {
    return *this;
  }
  delete fft_;
  sampleRate_ = other.sampleRate_;
  channels_ = other.channels_;
  dampingEnabled_ = other.dampingEnabled_;
  monoWindowed_ = other.monoWindowed_;
  magnitude_ = other.magnitude_;
  window_ = std::move(other.window_);
  energyHistory_ = std::move(other.energyHistory_);
  lastEnergy_ = other.lastEnergy_;
  lastBeatTimeSeconds_ = other.lastBeatTimeSeconds_;
  accumulatedTime_ = other.accumulatedTime_;
  framesProcessed_ = other.framesProcessed_;
  bpmSmoothing_ = other.bpmSmoothing_;
  confidenceSmoothing_ = other.confidenceSmoothing_;
  analysis_ = other.analysis_;
  fft_ = other.fft_;
  other.fft_ = nullptr;
  return *this;
}

void Analyzer::reset() {
  std::fill(monoWindowed_.begin(), monoWindowed_.end(), 0.0f);
  std::fill(magnitude_.begin(), magnitude_.end(), 0.0f);
  analysis_ = Analysis{};
  energyHistory_.clear();
  lastEnergy_ = 0.0f;
  lastBeatTimeSeconds_ = 0.0f;
  accumulatedTime_ = 0.0f;
  framesProcessed_ = 0;
  bpmSmoothing_ = 0.0f;
  confidenceSmoothing_ = 0.0f;
}

const Analysis& Analyzer::process(const float* interleavedSamples, std::size_t frameCount) {
  if (!interleavedSamples) {
    throw std::invalid_argument("Analyzer::process requires valid sample pointer");
  }
  if (frameCount != Analysis::kFftSize) {
    throw std::invalid_argument("Analyzer::process expects fixed frame size");
  }
  const float damping = dampingEnabled_ ? kDampingFactor : kNoDampingFactor;

  for (std::size_t i = 0; i < frameCount; ++i) {
    double sum = 0.0;
    for (int ch = 0; ch < channels_; ++ch) {
      sum += interleavedSamples[i * static_cast<std::size_t>(channels_) +
                                static_cast<std::size_t>(ch)];
    }
    const float mono = static_cast<float>(sum / static_cast<double>(channels_));
    const float windowed = mono * window_[i];
    monoWindowed_[i] = damping * monoWindowed_[i] + (1.0f - damping) * windowed;
  }

  updateSpectrum();
  updateWaveform();
  updateBands();
  updateBeat();

  accumulatedTime_ += static_cast<float>(frameCount) / static_cast<float>(sampleRate_);
  ++framesProcessed_;
  return analysis_;
}

void Analyzer::updateSpectrum() {
  kiss_fftr(fft_->cfg, monoWindowed_.data(), fft_->freq.data());
  for (std::size_t i = 0; i < Analysis::kSpectrumSize; ++i) {
    const kiss_fft_cpx& bin = fft_->freq[i];
    const float mag = std::sqrt(bin.r * bin.r + bin.i * bin.i);
    magnitude_[i] = mag;
    analysis_.spectrum[i] = mag;
  }
}

void Analyzer::updateWaveform() {
  const std::size_t hop = Analysis::kFftSize / Analysis::kWaveformSize;
  for (std::size_t i = 0; i < Analysis::kWaveformSize; ++i) {
    const std::size_t begin = i * hop;
    const std::size_t end = std::min(begin + hop, monoWindowed_.size());
    float sum = 0.0f;
    std::size_t count = 0;
    for (std::size_t j = begin; j < end; ++j) {
      sum += monoWindowed_[j];
      ++count;
    }
    const float value = count > 0 ? sum / static_cast<float>(count) : 0.0f;
    analysis_.waveform[i] = std::clamp(value, -1.0f, 1.0f);
  }
}

float Analyzer::hzForBin(std::size_t bin, int sampleRate) {
  return static_cast<float>(bin) * static_cast<float>(sampleRate) /
         static_cast<float>(Analysis::kFftSize);
}

void Analyzer::updateBands() {
  float bass = 0.0f;
  float mid = 0.0f;
  float treb = 0.0f;
  int bassCount = 0;
  int midCount = 0;
  int trebCount = 0;

  for (std::size_t i = 1; i < Analysis::kSpectrumSize; ++i) {
    const float hz = hzForBin(i, sampleRate_);
    const float mag = magnitude_[i];
    if (hz < 250.0f) {
      bass += mag;
      ++bassCount;
    } else if (hz < 4000.0f) {
      mid += mag;
      ++midCount;
    } else {
      treb += mag;
      ++trebCount;
    }
  }

  if (bassCount > 0) bass /= static_cast<float>(bassCount);
  if (midCount > 0) mid /= static_cast<float>(midCount);
  if (trebCount > 0) treb /= static_cast<float>(trebCount);

  const float smooth = dampingEnabled_ ? 0.5f : 0.0f;
  auto smoothValue = [smooth](float prev, float next) {
    return smooth > 0.0f ? lerp(prev, next, 1.0f - smooth) : next;
  };

  analysis_.bass = smoothValue(analysis_.bass, bass);
  analysis_.mid = smoothValue(analysis_.mid, mid);
  analysis_.treb = smoothValue(analysis_.treb, treb);
}

void Analyzer::updateBeat() {
  float energy = 0.0f;
  for (float v : monoWindowed_) {
    energy += v * v;
  }
  energy = std::max(energy, kMinEnergy);
  lastEnergy_ = energy;
  energyHistory_.push_back(energy);
  if (energyHistory_.size() > kEnergyWindow) {
    energyHistory_.pop_front();
  }

  const float avgEnergy = std::accumulate(energyHistory_.begin(), energyHistory_.end(), 0.0f) /
                          static_cast<float>(energyHistory_.empty() ? 1 : energyHistory_.size());
  float beatValue = 0.0f;
  if (avgEnergy > 0.0f) {
    beatValue = energy / avgEnergy;
  }
  const bool beat = beatValue > kBeatThreshold;
  analysis_.beat = beat;

  if (beat) {
    const float nowSeconds = accumulatedTime_;
    const float delta = nowSeconds - lastBeatTimeSeconds_;
    if (delta > 0.0f) {
      const float bpm = 60.0f / delta;
      bpmSmoothing_ = lerp(bpmSmoothing_, bpm, 0.35f);
      analysis_.bpm = bpmSmoothing_;
    }
    lastBeatTimeSeconds_ = nowSeconds;
  }

  const float confidence = std::min(kMaxConfidence, beatValue);
  confidenceSmoothing_ = lerp(confidenceSmoothing_, confidence, 0.25f);
  analysis_.confidence = confidenceSmoothing_ / kMaxConfidence;
}

}  // namespace avs::audio
