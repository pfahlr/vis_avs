#include "effects/render/effect_timescope.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "avs/audio/analyzer.h"

namespace avs::effects::render {

namespace {
constexpr int kMinBands = 16;
constexpr int kMaxBands = 576;
}

Timescope::Timescope() = default;

void Timescope::setParams(const avs::core::ParamBlock& params) {
  if (params.contains("enabled")) {
    enabled_ = params.getBool("enabled", enabled_);
  } else if (params.contains("bEnabled")) {
    enabled_ = params.getBool("bEnabled", enabled_);
  } else {
    enabled_ = params.getInt("enabled", enabled_ ? 1 : 0) != 0;
  }

  if (params.contains("color")) {
    color_ = static_cast<std::uint32_t>(params.getInt("color", static_cast<int>(color_)));
  } else if (params.contains("col")) {
    color_ = static_cast<std::uint32_t>(params.getInt("col", static_cast<int>(color_)));
  }

  if (params.contains("blendavg") && params.getInt("blendavg", 0) != 0) {
    blendMode_ = BlendMode::Average;
  } else {
    const int legacyBlend = params.getInt("blend", 2);
    switch (legacyBlend) {
      case 1:
        blendMode_ = BlendMode::Additive;
        break;
      case 2:
        blendMode_ = BlendMode::Line;
        break;
      default:
        blendMode_ = BlendMode::Replace;
        break;
    }
  }

  const int channel = params.getInt("which_ch", 2);
  switch (channel) {
    case 0:
      channel_ = Channel::Left;
      break;
    case 1:
      channel_ = Channel::Right;
      break;
    default:
      channel_ = Channel::Mix;
      break;
  }

  bands_ = params.getInt("nbands", bands_);
  bands_ = std::clamp(bands_, kMinBands, kMaxBands);

  lineBlendStrength_ = std::clamp(params.getInt("line_blend_strength", lineBlendStrength_), 0, 255);
}

bool Timescope::render(avs::core::RenderContext& context) {
  if (!context.framebuffer.data || context.width <= 0 || context.height <= 0) {
    return true;
  }

  const std::size_t required = static_cast<std::size_t>(context.width) *
                               static_cast<std::size_t>(context.height) * 4u;
  if (context.framebuffer.size < required) {
    return false;
  }

  if (!enabled_) {
    column_ = -1;
    return true;
  }

  if (column_ < 0 || column_ >= context.width) {
    column_ = 0;
  }
  column_ = (column_ + 1) % std::max(1, context.width);

  const avs::audio::Analysis* analysis = context.audioAnalysis;
  std::size_t availableSamples = 0;
  if (analysis) {
    availableSamples = analysis->waveform.size();
  }
  if (availableSamples == 0 && context.audioSpectrum.data && context.audioSpectrum.size > 0) {
    availableSamples = context.audioSpectrum.size;
  }
  if (availableSamples == 0) {
    return true;
  }

  const int effectiveBands = std::clamp(bands_, 1, static_cast<int>(availableSamples));
  const Rgba base = decodeColor(color_);

  const std::size_t rowStride = static_cast<std::size_t>(context.width) * 4u;
  std::uint8_t* columnPtr = context.framebuffer.data + static_cast<std::size_t>(column_) * 4u;

  for (int y = 0; y < context.height; ++y) {
    const std::size_t bandIndex = static_cast<std::size_t>(effectiveBands > 1
                                       ? (static_cast<std::size_t>(y) * (static_cast<std::size_t>(effectiveBands) - 1u)) /
                                             static_cast<std::size_t>(std::max(1, context.height - 1))
                                       : 0u);
    const std::size_t dataIndex = effectiveBands > 1
                                      ? std::min(availableSamples - 1u,
                                                 (bandIndex * (availableSamples - 1u)) /
                                                     static_cast<std::size_t>(effectiveBands - 1))
                                      : 0u;
    const float intensity = std::clamp(sampleValue(context, dataIndex), 0.0f, 1.0f);
    const Rgba color = scaleColor(base, intensity);

    std::uint8_t* pixel = columnPtr + static_cast<std::size_t>(y) * rowStride;
    switch (blendMode_) {
      case BlendMode::Additive:
        addPixel(pixel, color);
        break;
      case BlendMode::Average:
        averagePixel(pixel, color);
        break;
      case BlendMode::Line:
        lineBlendPixel(pixel, color);
        break;
      case BlendMode::Replace:
      default:
        writePixel(pixel, color);
        break;
    }
  }

  return true;
}

Timescope::Rgba Timescope::decodeColor(std::uint32_t color) {
  Rgba rgba{};
  rgba.r = static_cast<std::uint8_t>(color & 0xFFu);
  rgba.g = static_cast<std::uint8_t>((color >> 8) & 0xFFu);
  rgba.b = static_cast<std::uint8_t>((color >> 16) & 0xFFu);
  rgba.a = 255u;
  return rgba;
}

Timescope::Rgba Timescope::scaleColor(const Rgba& color, float intensity) {
  const float scaled = std::clamp(intensity, 0.0f, 1.0f);
  Rgba result{};
  result.r = static_cast<std::uint8_t>(std::lround(static_cast<float>(color.r) * scaled));
  result.g = static_cast<std::uint8_t>(std::lround(static_cast<float>(color.g) * scaled));
  result.b = static_cast<std::uint8_t>(std::lround(static_cast<float>(color.b) * scaled));
  result.a = color.a;
  return result;
}

std::uint8_t Timescope::clampByte(int value) {
  return static_cast<std::uint8_t>(std::clamp(value, 0, 255));
}

void Timescope::writePixel(std::uint8_t* pixel, const Rgba& color) const {
  pixel[0] = color.r;
  pixel[1] = color.g;
  pixel[2] = color.b;
  pixel[3] = color.a;
}

void Timescope::addPixel(std::uint8_t* pixel, const Rgba& color) const {
  pixel[0] = clampByte(static_cast<int>(pixel[0]) + static_cast<int>(color.r));
  pixel[1] = clampByte(static_cast<int>(pixel[1]) + static_cast<int>(color.g));
  pixel[2] = clampByte(static_cast<int>(pixel[2]) + static_cast<int>(color.b));
  pixel[3] = 255u;
}

void Timescope::averagePixel(std::uint8_t* pixel, const Rgba& color) const {
  pixel[0] = static_cast<std::uint8_t>((static_cast<int>(pixel[0]) + static_cast<int>(color.r)) / 2);
  pixel[1] = static_cast<std::uint8_t>((static_cast<int>(pixel[1]) + static_cast<int>(color.g)) / 2);
  pixel[2] = static_cast<std::uint8_t>((static_cast<int>(pixel[2]) + static_cast<int>(color.b)) / 2);
  pixel[3] = 255u;
}

void Timescope::lineBlendPixel(std::uint8_t* pixel, const Rgba& color) const {
  const int strength = std::clamp(lineBlendStrength_, 0, 255);
  const int inverse = 255 - strength;
  pixel[0] = static_cast<std::uint8_t>((static_cast<int>(pixel[0]) * inverse + static_cast<int>(color.r) * strength) /
                                       255);
  pixel[1] = static_cast<std::uint8_t>((static_cast<int>(pixel[1]) * inverse + static_cast<int>(color.g) * strength) /
                                       255);
  pixel[2] = static_cast<std::uint8_t>((static_cast<int>(pixel[2]) * inverse + static_cast<int>(color.b) * strength) /
                                       255);
  pixel[3] = 255u;
}

float Timescope::sampleValue(const avs::core::RenderContext& context, std::size_t index) const {
  const avs::audio::Analysis* analysis = context.audioAnalysis;
  if (analysis && !analysis->waveform.empty()) {
    const std::size_t total = analysis->waveform.size();
    const std::size_t clamped = std::min(index, total - 1u);
    const float sample = analysis->waveform[clamped];
    return std::clamp(std::fabs(sample), 0.0f, 1.0f);
  }

  if (context.audioSpectrum.data && context.audioSpectrum.size > 0) {
    const std::size_t total = context.audioSpectrum.size;
    const std::size_t clamped = std::min(index, total - 1u);
    const float sample = context.audioSpectrum.data[clamped];
    return std::clamp(sample, 0.0f, 1.0f);
  }

  return 0.0f;
}

}  // namespace avs::effects::render
