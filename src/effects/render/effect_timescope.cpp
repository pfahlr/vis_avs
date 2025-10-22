#include "effects/render/effect_timescope.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <string>
#include <string_view>

#include "audio/analyzer.h"
#include "avs/core/ParamBlock.hpp"
#include "avs/runtime/GlobalState.hpp"

namespace avs::effects::render {

namespace {
constexpr float kSpectrumFallbackScale = 32.0f;

Timescope::Channel parseChannelToken(std::string_view value, Timescope::Channel fallback) {
  std::string lower(value);
  std::transform(lower.begin(), lower.end(), lower.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  if (lower == "left" || lower == "l") {
    return Timescope::Channel::Left;
  }
  if (lower == "right" || lower == "r") {
    return Timescope::Channel::Right;
  }
  if (lower == "center" || lower == "centre" || lower == "mix" || lower == "mid") {
    return Timescope::Channel::Mix;
  }
  return fallback;
}

Timescope::Channel parseChannel(int value, Timescope::Channel fallback) {
  switch (value & 3) {
    case 0:
      return Timescope::Channel::Left;
    case 1:
      return Timescope::Channel::Right;
    case 2:
      return Timescope::Channel::Mix;
    default:
      return fallback;
  }
}

Timescope::BlendMode parseBlend(int value, Timescope::BlendMode fallback) {
  switch (value) {
    case 0:
      return Timescope::BlendMode::Replace;
    case 1:
      return Timescope::BlendMode::Additive;
    case 2:
      return Timescope::BlendMode::Line;
    default:
      return fallback;
  }
}

}  // namespace

Timescope::Timescope() { color_ = decodeColor(0x00FFFFFFu); }

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

  columnCursor_ = (columnCursor_ + 1) % std::max(1, context.width);

  const avs::runtime::LegacyRenderState* legacy =
      context.globals ? &context.globals->legacyRender : nullptr;
  const Operation op = resolveOperation(legacy);

  const std::size_t totalBands = std::max<std::size_t>(1, clampBands(bandCount_));

  for (int y = 0; y < context.height; ++y) {
    const float intensity = sampleBand(context,
                                       static_cast<int>((static_cast<std::size_t>(y) * totalBands) /
                                                        std::max<std::size_t>(1, context.height)),
                                       totalBands);
    const Rgba color = scaleColor(color_, intensity);
    std::uint8_t* pixel = context.framebuffer.data +
                          (static_cast<std::size_t>(y) * static_cast<std::size_t>(context.width) +
                           static_cast<std::size_t>(columnCursor_)) *
                              4u;
    switch (op) {
      case Operation::Replace:
        applyReplace(pixel, color);
        break;
      case Operation::Additive:
        applyAdditive(pixel, color);
        break;
      case Operation::Average:
        applyAverage(pixel, color);
        break;
      case Operation::Line:
        applyLine(pixel, color, legacy);
        break;
    }
  }

  return true;
}

void Timescope::setParams(const avs::core::ParamBlock& params) {
  if (params.contains("enabled")) {
    enabled_ = params.getInt("enabled", enabled_ ? 1 : 0) != 0;
  }

  if (params.contains("color")) {
    const std::string colorString = params.getString("color", "");
    Rgba parsed{};
    if (!colorString.empty() && parseColorString(colorString, parsed)) {
      color_ = parsed;
    } else {
      color_ = decodeColor(static_cast<std::uint32_t>(params.getInt("color", 0)));
    }
  } else {
    const int r = params.getInt("color_r", -1);
    const int g = params.getInt("color_g", -1);
    const int b = params.getInt("color_b", -1);
    if (r >= 0 && g >= 0 && b >= 0) {
      color_.r = static_cast<std::uint8_t>(std::clamp(r, 0, 255));
      color_.g = static_cast<std::uint8_t>(std::clamp(g, 0, 255));
      color_.b = static_cast<std::uint8_t>(std::clamp(b, 0, 255));
    }
  }

  if (params.contains("which_ch")) {
    channel_ = parseChannel(params.getInt("which_ch", 2), channel_);
  }
  if (params.contains("channel")) {
    channel_ = parseChannelToken(params.getString("channel", ""), channel_);
  }

  if (params.contains("blend")) {
    blendMode_ = parseBlend(params.getInt("blend", static_cast<int>(blendMode_)), blendMode_);
  }
  blendAverage_ = params.getBool("blendavg", blendAverage_);

  if (params.contains("nbands")) {
    bandCount_ = static_cast<int>(clampBands(params.getInt("nbands", bandCount_)));
  }
}

Timescope::Rgba Timescope::scaleColor(const Rgba& base, float intensity) {
  const float clamped = std::clamp(intensity, 0.0f, 1.0f);
  Rgba scaled{};
  scaled.r = static_cast<std::uint8_t>(std::lround(static_cast<float>(base.r) * clamped));
  scaled.g = static_cast<std::uint8_t>(std::lround(static_cast<float>(base.g) * clamped));
  scaled.b = static_cast<std::uint8_t>(std::lround(static_cast<float>(base.b) * clamped));
  scaled.a = static_cast<std::uint8_t>(std::lround(255.0f * clamped));
  return scaled;
}

Timescope::Rgba Timescope::decodeColor(std::uint32_t value) {
  Rgba color{};
  color.r = static_cast<std::uint8_t>((value >> 16u) & 0xFFu);
  color.g = static_cast<std::uint8_t>((value >> 8u) & 0xFFu);
  color.b = static_cast<std::uint8_t>(value & 0xFFu);
  color.a = 255u;
  return color;
}

std::uint32_t Timescope::clampBands(int requested) {
  const int minBands = 1;
  const int maxBands = static_cast<int>(avs::audio::Analysis::kWaveformSize);
  return static_cast<std::uint32_t>(std::clamp(requested, minBands, maxBands));
}

bool Timescope::parseColorString(std::string_view value, Rgba& color) {
  auto trim = [](std::string_view input) {
    std::string_view trimmed = input;
    while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.front()))) {
      trimmed.remove_prefix(1);
    }
    while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.back()))) {
      trimmed.remove_suffix(1);
    }
    return trimmed;
  };

  std::string_view trimmed = trim(value);
  if (trimmed.empty()) {
    return false;
  }

  int base = 10;
  if (trimmed.front() == '#') {
    base = 16;
    trimmed.remove_prefix(1);
  } else if (trimmed.size() > 2 && trimmed[0] == '0' && (trimmed[1] == 'x' || trimmed[1] == 'X')) {
    base = 16;
    trimmed.remove_prefix(2);
  }

  std::uint32_t parsed = 0;
  const char* begin = trimmed.data();
  const char* end = trimmed.data() + trimmed.size();
  const auto result = std::from_chars(begin, end, parsed, base);
  if (result.ec != std::errc() || result.ptr != end) {
    return false;
  }
  color = decodeColor(parsed);
  return true;
}

Timescope::Operation Timescope::resolveOperation(
    const avs::runtime::LegacyRenderState* legacy) const {
  if (blendMode_ == BlendMode::Additive) {
    return Operation::Additive;
  }
  if (blendMode_ == BlendMode::Replace) {
    return blendAverage_ ? Operation::Average : Operation::Replace;
  }
  if (blendMode_ == BlendMode::Line) {
    if (legacy && legacy->lineBlendModeActive) {
      return Operation::Line;
    }
    if (blendAverage_) {
      return Operation::Average;
    }
    return Operation::Additive;
  }
  return blendAverage_ ? Operation::Average : Operation::Replace;
}

void Timescope::applyReplace(std::uint8_t* pixel, const Rgba& color) {
  pixel[0] = color.r;
  pixel[1] = color.g;
  pixel[2] = color.b;
  pixel[3] = 255u;
}

void Timescope::applyAdditive(std::uint8_t* pixel, const Rgba& color) {
  pixel[0] = saturatingAdd(pixel[0], color.r);
  pixel[1] = saturatingAdd(pixel[1], color.g);
  pixel[2] = saturatingAdd(pixel[2], color.b);
  pixel[3] = saturatingAdd(pixel[3], color.a);
}

void Timescope::applyAverage(std::uint8_t* pixel, const Rgba& color) {
  pixel[0] = static_cast<std::uint8_t>((static_cast<int>(pixel[0]) + color.r) / 2);
  pixel[1] = static_cast<std::uint8_t>((static_cast<int>(pixel[1]) + color.g) / 2);
  pixel[2] = static_cast<std::uint8_t>((static_cast<int>(pixel[2]) + color.b) / 2);
  pixel[3] = static_cast<std::uint8_t>((static_cast<int>(pixel[3]) + color.a) / 2);
}

void Timescope::applyLine(std::uint8_t* pixel, const Rgba& color,
                          const avs::runtime::LegacyRenderState* legacy) {
  int mode = 0;
  int parameter = 0;
  if (legacy && legacy->lineBlendModeActive) {
    mode = static_cast<int>(legacy->lineBlendMode & 0xFFu);
    parameter = static_cast<int>((legacy->lineBlendMode >> 8u) & 0xFFu);
  }
  applyLineBlend(pixel, color, mode, parameter);
}

void Timescope::applyLineBlend(std::uint8_t* pixel, const Rgba& color, int mode, int parameter) {
  switch (mode) {
    case 0:
      applyReplace(pixel, color);
      break;
    case 1:
      applyAdditive(pixel, color);
      break;
    case 2:
      pixel[0] = std::max(pixel[0], color.r);
      pixel[1] = std::max(pixel[1], color.g);
      pixel[2] = std::max(pixel[2], color.b);
      pixel[3] = std::max(pixel[3], color.a);
      break;
    case 3:
      applyAverage(pixel, color);
      break;
    case 4:
      pixel[0] = saturatingSub(pixel[0], color.r);
      pixel[1] = saturatingSub(pixel[1], color.g);
      pixel[2] = saturatingSub(pixel[2], color.b);
      pixel[3] = saturatingSub(pixel[3], color.a);
      break;
    case 5:
      pixel[0] = saturatingSub(color.r, pixel[0]);
      pixel[1] = saturatingSub(color.g, pixel[1]);
      pixel[2] = saturatingSub(color.b, pixel[2]);
      pixel[3] = saturatingSub(color.a, pixel[3]);
      break;
    case 6:
      pixel[0] = multiplyChannel(pixel[0], color.r);
      pixel[1] = multiplyChannel(pixel[1], color.g);
      pixel[2] = multiplyChannel(pixel[2], color.b);
      pixel[3] = multiplyChannel(pixel[3], color.a);
      break;
    case 7: {
      const int alpha = std::clamp(parameter, 0, 255);
      const int inv = 255 - alpha;
      pixel[0] = static_cast<std::uint8_t>((pixel[0] * inv + color.r * alpha) / 255);
      pixel[1] = static_cast<std::uint8_t>((pixel[1] * inv + color.g * alpha) / 255);
      pixel[2] = static_cast<std::uint8_t>((pixel[2] * inv + color.b * alpha) / 255);
      pixel[3] = static_cast<std::uint8_t>((pixel[3] * inv + color.a * alpha) / 255);
      break;
    }
    case 8:
      pixel[0] ^= color.r;
      pixel[1] ^= color.g;
      pixel[2] ^= color.b;
      pixel[3] ^= color.a;
      break;
    case 9:
      pixel[0] = std::min(pixel[0], color.r);
      pixel[1] = std::min(pixel[1], color.g);
      pixel[2] = std::min(pixel[2], color.b);
      pixel[3] = std::min(pixel[3], color.a);
      break;
    default:
      applyAdditive(pixel, color);
      break;
  }
}

std::uint8_t Timescope::saturatingAdd(std::uint8_t base, std::uint8_t add) {
  const int value = static_cast<int>(base) + static_cast<int>(add);
  return static_cast<std::uint8_t>(value > 255 ? 255 : value);
}

std::uint8_t Timescope::saturatingSub(std::uint8_t base, std::uint8_t sub) {
  const int value = static_cast<int>(base) - static_cast<int>(sub);
  return static_cast<std::uint8_t>(value < 0 ? 0 : value);
}

std::uint8_t Timescope::multiplyChannel(std::uint8_t a, std::uint8_t b) {
  return static_cast<std::uint8_t>((static_cast<int>(a) * static_cast<int>(b)) / 255);
}

float Timescope::sampleBand(const avs::core::RenderContext& context, int band,
                            std::size_t totalBands) const {
  if (const avs::audio::Analysis* analysis = context.audioAnalysis) {
    const auto& waveform = analysis->waveform;
    if (!waveform.empty()) {
      return std::clamp(std::abs(sampleWaveform(waveform.data(), waveform.size(),
                                                static_cast<std::size_t>(band), totalBands)),
                        0.0f, 1.0f);
    }
  }

  if (context.audioSpectrum.data && context.audioSpectrum.size > 0) {
    const float magnitude = sampleSpectrum(context.audioSpectrum.data, context.audioSpectrum.size,
                                           static_cast<std::size_t>(band), totalBands);
    return std::clamp(magnitude / kSpectrumFallbackScale, 0.0f, 1.0f);
  }

  return 0.0f;
}

float Timescope::sampleWaveform(const float* waveform, std::size_t size, std::size_t band,
                                std::size_t totalBands) const {
  if (!waveform || size == 0) {
    return 0.0f;
  }
  const auto [offset, count] = channelRange(size);
  if (count == 0) {
    return 0.0f;
  }
  const std::size_t localBands = std::max<std::size_t>(1, totalBands);
  const std::size_t indexInRange = std::min(count - 1, (band * count) / localBands);
  return waveform[offset + indexInRange];
}

float Timescope::sampleSpectrum(const float* spectrum, std::size_t size, std::size_t band,
                                std::size_t totalBands) const {
  if (!spectrum || size == 0) {
    return 0.0f;
  }
  const auto [offset, count] = channelRange(size);
  if (count == 0) {
    return 0.0f;
  }
  const std::size_t localBands = std::max<std::size_t>(1, totalBands);
  const std::size_t indexInRange = std::min(count - 1, (band * count) / localBands);
  return std::max(0.0f, spectrum[offset + indexInRange]);
}

std::pair<std::size_t, std::size_t> Timescope::channelRange(std::size_t total) const {
  if (total == 0) {
    return {0, 0};
  }
  switch (channel_) {
    case Channel::Left: {
      const std::size_t half = total / 2;
      const std::size_t count = std::max<std::size_t>(1, half);
      return {0, count};
    }
    case Channel::Right: {
      const std::size_t half = total / 2;
      const std::size_t offset = half;
      const std::size_t count = std::max<std::size_t>(1, total - offset);
      return {offset, count};
    }
    case Channel::Mix:
    default:
      return {0, total};
  }
}

}  // namespace avs::effects::render
