#include "OffscreenRenderer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <utility>

#include "avs/audio.hpp"
#include "avs/engine.hpp"
#include "avs/fft.hpp"
#include "avs/preset.hpp"

namespace avs::offscreen {

namespace {
constexpr int kFftSize = 2048;
constexpr float kBandSmooth = 0.2f;
}

struct OffscreenRenderer::AudioTrack {
  AudioTrack(std::vector<float> samplesIn, unsigned sampleRateIn, unsigned channelsIn)
      : samples(std::move(samplesIn)),
        sampleRate(sampleRateIn),
        channels(std::max(1u, channelsIn)),
        fft(kFftSize),
        mono(kFftSize, 0.0f),
        spectrum(kFftSize / 2, 0.0f) {}

  avs::AudioState next(double deltaSeconds);
  void reset();

  std::vector<float> samples;
  unsigned sampleRate = 0;
  unsigned channels = 0;
  std::uint64_t position = 0;
  avs::FFT fft;
  std::vector<float> mono;
  std::vector<float> spectrum;
  std::array<float, 3> bands{{0.f, 0.f, 0.f}};
};

avs::AudioState OffscreenRenderer::AudioTrack::next(double deltaSeconds) {
  avs::AudioState state;
  if (sampleRate == 0 || channels == 0) {
    return state;
  }

  std::size_t framesPerStep = 0;
  if (deltaSeconds > 0.0) {
    const double frames = static_cast<double>(sampleRate) * deltaSeconds;
    framesPerStep = static_cast<std::size_t>(std::llround(frames));
  }
  if (framesPerStep == 0) {
    framesPerStep = static_cast<std::size_t>(sampleRate / 60u);
  }
  if (framesPerStep == 0) {
    framesPerStep = 1;
  }
  const std::size_t step = framesPerStep * static_cast<std::size_t>(channels);

  position += step;
  const long long needed = static_cast<long long>(kFftSize) * static_cast<long long>(channels);
  const long long start = static_cast<long long>(position) - needed;

  for (int i = 0; i < kFftSize; ++i) {
    double sum = 0.0;
    for (unsigned c = 0; c < channels; ++c) {
      const long long idx = start + static_cast<long long>(i) * static_cast<long long>(channels) +
                            static_cast<long long>(c);
      float sample = 0.0f;
      if (idx >= 0 && idx < static_cast<long long>(samples.size())) {
        sample = samples[static_cast<std::size_t>(idx)];
      }
      sum += sample;
    }
    mono[static_cast<std::size_t>(i)] = static_cast<float>(sum / static_cast<double>(channels));
  }

  double sumSq = 0.0;
  for (float v : mono) {
    sumSq += static_cast<double>(v) * static_cast<double>(v);
  }
  state.rms = static_cast<float>(std::sqrt(sumSq / static_cast<double>(kFftSize)));

  fft.compute(mono.data(), spectrum);
  state.spectrum = spectrum;

  avs::AudioState::LegacyBuffer specLegacy{};
  specLegacy.fill(0.0f);
  const std::size_t legacyCount = avs::AudioState::kLegacyVisSamples;
  const std::size_t spectrumSize = spectrum.size();
  if (legacyCount > 0 && spectrumSize > 0) {
    const double scale = static_cast<double>(spectrumSize) / static_cast<double>(legacyCount);
    for (std::size_t i = 0; i < legacyCount; ++i) {
      const double beginF = static_cast<double>(i) * scale;
      const double endF = static_cast<double>(i + 1) * scale;
      std::size_t begin = static_cast<std::size_t>(std::floor(beginF));
      std::size_t end = static_cast<std::size_t>(std::floor(endF));
      if (end <= begin) {
        end = std::min(begin + 1, spectrumSize);
      }
      double accum = 0.0;
      std::size_t count = 0;
      for (std::size_t j = begin; j < end && j < spectrumSize; ++j) {
        accum += spectrum[j];
        ++count;
      }
      float value = 0.0f;
      if (count > 0) {
        value = static_cast<float>(accum / static_cast<double>(count));
      } else if (begin < spectrumSize) {
        value = spectrum[begin];
      }
      specLegacy[i] = value;
    }
  }
  state.spectrumLegacy[0] = specLegacy;
  state.spectrumLegacy[1] = specLegacy;

  for (auto& scope : state.oscilloscope) {
    scope.fill(0.0f);
  }
  const std::size_t legacySamples = avs::AudioState::kLegacyVisSamples;
  if (legacySamples > 0) {
    const std::size_t sampleStart = kFftSize > legacySamples ? static_cast<std::size_t>(kFftSize) - legacySamples : 0;
    const std::size_t channelsToCopy = std::min<std::size_t>(2, channels);
    for (std::size_t ch = 0; ch < channelsToCopy; ++ch) {
      auto& dest = state.oscilloscope[ch];
      for (std::size_t i = 0; i < legacySamples; ++i) {
        const std::size_t sampleIndex = sampleStart + i;
        if (sampleIndex >= static_cast<std::size_t>(kFftSize)) {
          break;
        }
        const long long idx = start + static_cast<long long>(sampleIndex) * static_cast<long long>(channels) +
                              static_cast<long long>(ch);
        float sample = 0.0f;
        if (idx >= 0 && idx < static_cast<long long>(samples.size())) {
          sample = samples[static_cast<std::size_t>(idx)];
        }
        dest[i] = sample;
      }
    }
    if (channels == 1) {
      state.oscilloscope[1] = state.oscilloscope[0];
    }
  }

  std::array<float, 3> newBands{0.f, 0.f, 0.f};
  std::array<int, 3> counts{0, 0, 0};
  const double binHz = static_cast<double>(sampleRate) / static_cast<double>(kFftSize);
  for (std::size_t i = 0; i < spectrum.size(); ++i) {
    const double freq = static_cast<double>(i) * binHz;
    const float mag = spectrum[i];
    if (freq < 250.0) {
      newBands[0] += mag;
      counts[0]++;
    } else if (freq < 4000.0) {
      newBands[1] += mag;
      counts[1]++;
    } else {
      newBands[2] += mag;
      counts[2]++;
    }
  }
  for (int i = 0; i < 3; ++i) {
    if (counts[i] > 0) {
      newBands[i] /= static_cast<float>(counts[i]);
    }
    bands[i] = bands[i] * (1.0f - kBandSmooth) + newBands[i] * kBandSmooth;
  }
  state.bands = bands;

  const double denom = static_cast<double>(channels) * static_cast<double>(sampleRate);
  if (denom > 0.0) {
    state.timeSeconds = static_cast<double>(position) / denom;
  } else {
    state.timeSeconds = 0.0;
  }
  state.sampleRate = static_cast<int>(sampleRate);
  state.inputSampleRate = static_cast<int>(sampleRate);
  state.channels = static_cast<int>(channels);
  return state;
}

void OffscreenRenderer::AudioTrack::reset() {
  position = 0;
  bands = {0.f, 0.f, 0.f};
}

OffscreenRenderer::OffscreenRenderer(int width, int height)
    : width_(width), height_(height), engine_(std::make_unique<avs::Engine>(width, height)) {}

OffscreenRenderer::~OffscreenRenderer() = default;

void OffscreenRenderer::loadPreset(const std::filesystem::path& presetPath) {
  auto parsed = avs::parsePreset(presetPath);
  if (parsed.chain.empty()) {
    throw std::runtime_error("Preset contains no effects: " + presetPath.string());
  }

  presetPath_ = presetPath;
  engine_ = std::make_unique<avs::Engine>(width_, height_);
  engine_->setChain(std::move(parsed.chain));
  presetLoaded_ = true;
  frameIndex_ = 0;
  if (audio_) {
    audio_->reset();
  }
}

void OffscreenRenderer::setAudioBuffer(std::vector<float> samples, unsigned sampleRate, unsigned channels) {
  if (sampleRate == 0 || channels == 0) {
    audio_.reset();
    return;
  }
  audio_ = std::make_unique<AudioTrack>(std::move(samples), sampleRate, channels);
  if (presetLoaded_) {
    audio_->reset();
  }
}

FrameView OffscreenRenderer::render() {
  if (!engine_) {
    engine_ = std::make_unique<avs::Engine>(width_, height_);
  }
  if (!presetLoaded_) {
    throw std::runtime_error("OffscreenRenderer requires a preset before rendering");
  }

  avs::AudioState audioState;
  if (audio_) {
    audioState = audio_->next(deltaSeconds_);
  }
  engine_->setAudio(audioState);
  engine_->step(static_cast<float>(deltaSeconds_));
  FrameView view = currentFrame();
  ++frameIndex_;
  return view;
}

FrameView OffscreenRenderer::currentFrame() const {
  FrameView view;
  if (!engine_) {
    return view;
  }
  const auto& fb = engine_->frame();
  view.data = fb.rgba.data();
  view.size = fb.rgba.size();
  view.width = fb.w;
  view.height = fb.h;
  return view;
}

}  // namespace avs::offscreen

