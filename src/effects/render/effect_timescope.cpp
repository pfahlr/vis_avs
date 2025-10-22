#include "effects/render/effect_timescope.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

#include "audio/analyzer.h"
#include "avs/core/RenderContext.hpp"
#include "avs/runtime/GlobalState.hpp"
#include "effects/render/primitive_common.hpp"

namespace avs::effects::render {

namespace {

constexpr float kSmoothingFactor = 0.65f;
constexpr float kPeakRiseRate = 0.25f;
constexpr float kPeakDecayRate = 0.92f;

std::uint32_t sanitizeColor(std::uint32_t value) { return value & 0x00FFFFFFu; }

int clampBands(int bands) {
  if (bands < 1) {
    return 1;
  }
  if (bands > static_cast<int>(avs::audio::Analysis::kWaveformSize)) {
    return static_cast<int>(avs::audio::Analysis::kWaveformSize);
  }
  return bands;
}

}  // namespace

Timescope::Timescope() { updateColor(0x00FFFFFFu); }

void Timescope::setParams(const avs::core::ParamBlock& params) {
  if (params.contains("enabled")) {
    enabled_ = params.getInt("enabled", enabled_ ? 1 : 0) != 0;
  }

  if (params.contains("color")) {
    updateColor(sanitizeColor(static_cast<std::uint32_t>(params.getInt("color", 0))));
  }

  if (params.contains("blendavg")) {
    if (params.getInt("blendavg", 0) != 0) {
      blendMode_ = BlendMode::Average;
    }
  }

  if (params.contains("blend")) {
    const int blend = params.getInt("blend", 0);
    switch (blend) {
      case 1:
        blendMode_ = BlendMode::Additive;
        break;
      case 2:
        blendMode_ = BlendMode::Line;
        break;
      default:
        if (blendMode_ != BlendMode::Average) {
          blendMode_ = BlendMode::Replace;
        }
        break;
    }
  }

  if (params.contains("blend_mode")) {
    const int blend = params.getInt("blend_mode", static_cast<int>(BlendMode::Line));
    switch (blend) {
      case 0:
        blendMode_ = BlendMode::Replace;
        break;
      case 1:
        blendMode_ = BlendMode::Additive;
        break;
      case 2:
        blendMode_ = BlendMode::Average;
        break;
      default:
        blendMode_ = BlendMode::Line;
        break;
    }
  }

  if (params.contains("which_ch")) {
    channelMode_ = std::clamp(params.getInt("which_ch", channelMode_), 0, 2);
  } else if (params.contains("channel")) {
    channelMode_ = std::clamp(params.getInt("channel", channelMode_), 0, 2);
  }

  if (params.contains("nbands")) {
    bandCount_ = clampBands(params.getInt("nbands", bandCount_));
  } else if (params.contains("bands")) {
    bandCount_ = clampBands(params.getInt("bands", bandCount_));
  }
}

bool Timescope::render(avs::core::RenderContext& context) {
  if (!enabled_) {
    return true;
  }
  if (!context.framebuffer.data || context.width <= 0 || context.height <= 0) {
    return true;
  }

  const std::size_t required =
      static_cast<std::size_t>(context.width) * static_cast<std::size_t>(context.height) * 4u;
  if (context.framebuffer.size < required) {
    return false;
  }

  if (context.width != lastWidth_) {
    resetColumnState(context.width);
  }

  advanceColumn(context.width);
  const int column = currentColumn_;

  const std::size_t bands = static_cast<std::size_t>(clampBands(bandCount_));
  ensureBandStorage(bands);
  updateSpectrumColumn(context, bands);

  for (int y = 0; y < context.height; ++y) {
    const std::uint8_t intensity = bandIntensityForRow(y, context.height);
    drawPixel(context, column, y, intensity);
  }

  return true;
}

void Timescope::updateColor(std::uint32_t color) {
  baseColor_[0] = static_cast<std::uint8_t>((color >> 16) & 0xFFu);
  baseColor_[1] = static_cast<std::uint8_t>((color >> 8) & 0xFFu);
  baseColor_[2] = static_cast<std::uint8_t>(color & 0xFFu);
}

void Timescope::resetColumnState(int width) {
  currentColumn_ = width > 0 ? (width - 1) : 0;
  lastWidth_ = width;
}

void Timescope::advanceColumn(int width) {
  if (width <= 0) {
    currentColumn_ = 0;
    return;
  }
  currentColumn_ = (currentColumn_ + 1) % width;
}

void Timescope::ensureBandStorage(std::size_t bands) {
  if (columnBuffer_.size() != bands) {
    columnBuffer_.assign(bands, 0u);
  }
  if (smoothedBands_.size() != bands) {
    smoothedBands_.assign(bands, 0.0f);
  }
}

void Timescope::updateSpectrumColumn(const avs::core::RenderContext& context, std::size_t bands) {
  const float* spectrumData = nullptr;
  std::size_t spectrumSize = 0;

  if (context.audioSpectrum.data && context.audioSpectrum.size > 0) {
    spectrumData = context.audioSpectrum.data;
    spectrumSize = context.audioSpectrum.size;
  } else if (context.audioAnalysis) {
    spectrumData = context.audioAnalysis->spectrum.data();
    spectrumSize = context.audioAnalysis->spectrum.size();
  }

  if (!spectrumData || spectrumSize == 0) {
    for (std::size_t i = 0; i < bands; ++i) {
      smoothedBands_[i] *= kPeakDecayRate;
      columnBuffer_[i] =
          static_cast<std::uint8_t>(std::clamp(smoothedBands_[i] * 255.0f, 0.0f, 255.0f));
    }
    magnitudePeak_ = std::max(1.0f, magnitudePeak_ * kPeakDecayRate);
    return;
  }

  float framePeak = 0.0f;
  for (std::size_t i = 0; i < spectrumSize; ++i) {
    framePeak = std::max(framePeak, spectrumData[i]);
  }

  if (framePeak <= std::numeric_limits<float>::epsilon()) {
    framePeak = magnitudePeak_ * kPeakDecayRate;
  }

  if (framePeak > magnitudePeak_) {
    magnitudePeak_ = framePeak * (1.0f - kPeakRiseRate) + magnitudePeak_ * kPeakRiseRate;
  } else {
    magnitudePeak_ = magnitudePeak_ * kPeakDecayRate + framePeak * (1.0f - kPeakDecayRate);
  }
  if (magnitudePeak_ < std::numeric_limits<float>::epsilon()) {
    magnitudePeak_ = 1.0f;
  }

  const float invPeak = 1.0f / magnitudePeak_;
  const float denom = spectrumSize > 1 ? static_cast<float>(spectrumSize - 1) : 1.0f;

  for (std::size_t band = 0; band < bands; ++band) {
    const float position =
        bands > 1 ? static_cast<float>(band) / static_cast<float>(bands - 1) : 0.0f;
    const float sampleIndex = position * denom;
    const std::size_t base = static_cast<std::size_t>(std::floor(sampleIndex));
    const std::size_t next = std::min(base + 1, spectrumSize - 1);
    const float frac = sampleIndex - static_cast<float>(base);
    const float sample = spectrumData[base] * (1.0f - frac) + spectrumData[next] * frac;
    const float normalized = std::clamp(sample * invPeak, 0.0f, 1.0f);
    const float eased = std::sqrt(normalized);
    smoothedBands_[band] =
        smoothedBands_[band] * kSmoothingFactor + eased * (1.0f - kSmoothingFactor);
    const float scaled = std::clamp(smoothedBands_[band] * 255.0f, 0.0f, 255.0f);
    columnBuffer_[band] = static_cast<std::uint8_t>(scaled);
  }
}

std::uint8_t Timescope::bandIntensityForRow(int row, int height) const {
  if (height <= 0 || columnBuffer_.empty()) {
    return 0;
  }
  const std::size_t bands = columnBuffer_.size();
  const std::size_t index =
      static_cast<std::size_t>(static_cast<long long>(row) * static_cast<long long>(bands)) /
      static_cast<std::size_t>(std::max(height, 1));
  const std::size_t clamped = std::min<std::size_t>(index, bands - 1);
  return columnBuffer_[clamped];
}

std::array<std::uint8_t, 3> Timescope::scaledColor(std::uint8_t intensity) const {
  std::array<std::uint8_t, 3> color{};
  for (std::size_t i = 0; i < color.size(); ++i) {
    const int scaled = static_cast<int>(baseColor_[i]) * static_cast<int>(intensity);
    color[i] = static_cast<std::uint8_t>((scaled + 127) / 255);
  }
  return color;
}

void Timescope::drawPixel(avs::core::RenderContext& context, int x, int y,
                          std::uint8_t intensity) const {
  if (x < 0 || y < 0 || x >= context.width || y >= context.height) {
    return;
  }
  const std::size_t index = (static_cast<std::size_t>(y) * static_cast<std::size_t>(context.width) +
                             static_cast<std::size_t>(x)) *
                            4u;
  if (index + 3u >= context.framebuffer.size) {
    return;
  }

  const auto color = scaledColor(intensity);
  auto* pixel = context.framebuffer.data + index;

  switch (blendMode_) {
    case BlendMode::Replace:
      pixel[0] = color[0];
      pixel[1] = color[1];
      pixel[2] = color[2];
      pixel[3] = 255u;
      break;
    case BlendMode::Additive:
      pixel[0] = avs::effects::detail::saturatingAdd(pixel[0], color[0]);
      pixel[1] = avs::effects::detail::saturatingAdd(pixel[1], color[1]);
      pixel[2] = avs::effects::detail::saturatingAdd(pixel[2], color[2]);
      pixel[3] = avs::effects::detail::saturatingAdd(pixel[3], intensity);
      break;
    case BlendMode::Average:
      pixel[0] = avs::effects::detail::averageChannel(pixel[0], color[0]);
      pixel[1] = avs::effects::detail::averageChannel(pixel[1], color[1]);
      pixel[2] = avs::effects::detail::averageChannel(pixel[2], color[2]);
      pixel[3] = avs::effects::detail::averageChannel(pixel[3], intensity);
      break;
    case BlendMode::Line: {
      const bool legacyActive =
          context.globals && context.globals->legacyRender.lineBlendModeActive;
      if (legacyActive) {
        avs::effects::detail::RGBA rgba{color[0], color[1], color[2], 255};
        avs::effects::detail::blendPixel(context, x, y, rgba);
      } else {
        pixel[0] = color[0];
        pixel[1] = color[1];
        pixel[2] = color[2];
        pixel[3] = 255u;
      }
      break;
    }
  }
}

}  // namespace avs::effects::render
