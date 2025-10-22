#include "effects/render/effect_timescope.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "audio/analyzer.h"

namespace avs::effects::render {

namespace {
constexpr int kMaxBands = static_cast<int>(avs::audio::Analysis::kWaveformSize);
}

Timescope::Timescope() = default;

std::uint8_t Timescope::clampByte(int value) {
  if (value < 0) {
    return 0u;
  }
  if (value > 255) {
    return 255u;
  }
  return static_cast<std::uint8_t>(value);
}

std::uint8_t Timescope::saturatingAdd(std::uint8_t a, std::uint8_t b) {
  const int sum = static_cast<int>(a) + static_cast<int>(b);
  return static_cast<std::uint8_t>(sum > 255 ? 255 : sum);
}

Timescope::Color Timescope::decodeColor(std::uint32_t value) {
  Color color{};
  color.r = static_cast<std::uint8_t>((value >> 16u) & 0xFFu);
  color.g = static_cast<std::uint8_t>((value >> 8u) & 0xFFu);
  color.b = static_cast<std::uint8_t>(value & 0xFFu);
  return color;
}

std::uint32_t Timescope::encodeColor(const Color& color) {
  return static_cast<std::uint32_t>(color.r) << 16u | static_cast<std::uint32_t>(color.g) << 8u |
         static_cast<std::uint32_t>(color.b);
}

std::uint8_t Timescope::intensityFromSample(float sample) {
  const float clamped = std::clamp(sample, -1.0f, 1.0f);
  const float scaled = (clamped + 1.0f) * 127.5f;
  const int value = static_cast<int>(std::lround(scaled));
  return clampByte(value);
}

std::string Timescope::toLower(std::string_view value) {
  std::string result(value);
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return result;
}

void Timescope::resetState() {
  column_ = -1;
  lastWidth_ = 0;
}

void Timescope::applyBlend(std::uint8_t* pixel, const Rgba& src) const {
  switch (blendMode_) {
    case BlendMode::Replace: {
      pixel[0] = src.r;
      pixel[1] = src.g;
      pixel[2] = src.b;
      pixel[3] = 255u;
      break;
    }
    case BlendMode::Additive: {
      pixel[0] = saturatingAdd(pixel[0], src.r);
      pixel[1] = saturatingAdd(pixel[1], src.g);
      pixel[2] = saturatingAdd(pixel[2], src.b);
      pixel[3] = std::max(pixel[3], src.a);
      break;
    }
    case BlendMode::Average: {
      pixel[0] = static_cast<std::uint8_t>((static_cast<int>(pixel[0]) + src.r) / 2);
      pixel[1] = static_cast<std::uint8_t>((static_cast<int>(pixel[1]) + src.g) / 2);
      pixel[2] = static_cast<std::uint8_t>((static_cast<int>(pixel[2]) + src.b) / 2);
      pixel[3] = std::max(pixel[3], src.a);
      break;
    }
    case BlendMode::Alpha: {
      const std::uint8_t alpha = src.a;
      const std::uint8_t inv = static_cast<std::uint8_t>(255u - alpha);
      pixel[0] = static_cast<std::uint8_t>(
          (static_cast<int>(pixel[0]) * inv + static_cast<int>(src.r) * alpha) / 255);
      pixel[1] = static_cast<std::uint8_t>(
          (static_cast<int>(pixel[1]) * inv + static_cast<int>(src.g) * alpha) / 255);
      pixel[2] = static_cast<std::uint8_t>(
          (static_cast<int>(pixel[2]) * inv + static_cast<int>(src.b) * alpha) / 255);
      pixel[3] = std::max(pixel[3], alpha);
      break;
    }
  }
}

Timescope::Rgba Timescope::colorForIntensity(std::uint8_t intensity) const {
  Rgba color{};
  color.r = static_cast<std::uint8_t>((static_cast<int>(color_.r) * intensity) / 255);
  color.g = static_cast<std::uint8_t>((static_cast<int>(color_.g) * intensity) / 255);
  color.b = static_cast<std::uint8_t>((static_cast<int>(color_.b) * intensity) / 255);
  color.a = intensity;
  return color;
}

Timescope::SampleView Timescope::resolveWaveform(const avs::core::RenderContext& context) const {
  if (context.audioAnalysis) {
    const auto* analysis = context.audioAnalysis;
    return {analysis->waveform.data(), analysis->waveform.size()};
  }
  return {};
}

std::size_t Timescope::sampleIndexForRow(int row, int height, std::size_t available) const {
  if (height <= 0 || available == 0) {
    return 0;
  }
  const int effectiveBands = std::clamp(bands_, 1, static_cast<int>(available));
  const long long scaled = static_cast<long long>(row) * static_cast<long long>(effectiveBands);
  int index = static_cast<int>(scaled / static_cast<long long>(height));
  if (index < 0) {
    index = 0;
  }
  if (index >= effectiveBands) {
    index = effectiveBands - 1;
  }
  return static_cast<std::size_t>(index);
}

void Timescope::setParams(const avs::core::ParamBlock& params) {
  if (params.contains("enabled")) {
    enabled_ = params.getBool("enabled", enabled_);
  }

  if (params.contains("color")) {
    color_ = decodeColor(
        static_cast<std::uint32_t>(params.getInt("color", static_cast<int>(encodeColor(color_)))));
  }
  if (params.contains("color_r")) {
    color_.r = clampByte(params.getInt("color_r", color_.r));
  }
  if (params.contains("color_g")) {
    color_.g = clampByte(params.getInt("color_g", color_.g));
  }
  if (params.contains("color_b")) {
    color_.b = clampByte(params.getInt("color_b", color_.b));
  }

  if (params.contains("blend_mode")) {
    const std::string value = toLower(params.getString("blend_mode", "alpha"));
    if (value == "replace") {
      blendMode_ = BlendMode::Replace;
    } else if (value == "add" || value == "additive") {
      blendMode_ = BlendMode::Additive;
    } else if (value == "avg" || value == "average") {
      blendMode_ = BlendMode::Average;
    } else {
      blendMode_ = BlendMode::Alpha;
    }
  } else if (params.contains("blend") || params.contains("blendavg")) {
    const int blendInt = params.getInt("blend", 0);
    const bool blendAvg = params.getBool("blendavg", false);
    if (blendInt == 1) {
      blendMode_ = BlendMode::Additive;
    } else if (blendInt == 2) {
      blendMode_ = BlendMode::Alpha;
    } else if (blendAvg) {
      blendMode_ = BlendMode::Average;
    } else {
      blendMode_ = BlendMode::Replace;
    }
  }

  if (params.contains("bands")) {
    bands_ = std::clamp(params.getInt("bands", bands_), 1, kMaxBands);
  } else if (params.contains("nbands")) {
    bands_ = std::clamp(params.getInt("nbands", bands_), 1, kMaxBands);
  }

  resetState();
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

  SampleView waveform = resolveWaveform(context);
  if (!waveform.data || waveform.size == 0) {
    return true;
  }

  if (context.width != lastWidth_) {
    column_ = -1;
    lastWidth_ = context.width;
  }
  column_ = (column_ + 1) % context.width;

  for (int y = 0; y < context.height; ++y) {
    const std::size_t sampleIndex = sampleIndexForRow(y, context.height, waveform.size);
    const float sample = waveform.data[sampleIndex];
    const std::uint8_t intensity = intensityFromSample(sample);
    const Rgba color = colorForIntensity(intensity);

    const std::size_t offset =
        (static_cast<std::size_t>(y) * static_cast<std::size_t>(context.width) +
         static_cast<std::size_t>(column_)) *
        4u;
    std::uint8_t* pixel = context.framebuffer.data + offset;
    applyBlend(pixel, color);
  }

  return true;
}

}  // namespace avs::effects::render
