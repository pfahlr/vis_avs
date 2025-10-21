#include "effects/render/effect_timescope.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace avs::effects::render {
namespace {

constexpr int kMinSampleCount = 16;
constexpr int kWaveformSize = static_cast<int>(avs::audio::Analysis::kWaveformSize);
constexpr std::uint8_t clampByte(int value) {
  return static_cast<std::uint8_t>(std::clamp(value, 0, 255));
}

float parseThickness(const avs::core::ParamBlock& params, float fallback) {
  const float value = params.getFloat("thickness", fallback);
  return std::clamp(value, 0.001f, 1.0f);
}

int parseChannel(const avs::core::ParamBlock& params, int fallback) {
  const int channel = params.getInt("which_ch", params.getInt("channel", fallback));
  if (channel < 0) {
    return 0;
  }
  if (channel > 2) {
    return 2;
  }
  return channel;
}

bool parseEnabled(const avs::core::ParamBlock& params, bool fallback) {
  if (params.contains("enabled")) {
    if (params.getBool("enabled", fallback)) {
      return true;
    }
    return params.getInt("enabled", fallback ? 1 : 0) != 0;
  }
  if (params.contains("active")) {
    return params.getBool("active", fallback);
  }
  return fallback;
}

int parseSampleCount(const avs::core::ParamBlock& params, int fallback) {
  const int value = params.getInt("nbands", params.getInt("samples", fallback));
  return std::clamp(value, kMinSampleCount, kWaveformSize);
}

}  // namespace

Timescope::Timescope() = default;

void Timescope::updateBlendModeFromParams(const avs::core::ParamBlock& params) {
  const std::string modeToken = params.getString("blend_mode", "");
  if (!modeToken.empty()) {
    if (modeToken == "add" || modeToken == "additive") {
      blendMode_ = BlendMode::Additive;
      return;
    }
    if (modeToken == "avg" || modeToken == "average") {
      blendMode_ = BlendMode::Average;
      return;
    }
    if (modeToken == "max" || modeToken == "lighten") {
      blendMode_ = BlendMode::Max;
      return;
    }
    blendMode_ = BlendMode::Replace;
    return;
  }

  const int legacyBlend = params.getInt("blend", 0);
  const bool legacyAvg = params.getBool("blendavg", false) || params.getInt("blendavg", 0) != 0;
  if (legacyBlend == 1) {
    blendMode_ = BlendMode::Additive;
  } else if (legacyBlend == 2) {
    blendMode_ = BlendMode::Max;
  } else if (legacyAvg) {
    blendMode_ = BlendMode::Average;
  } else {
    blendMode_ = BlendMode::Replace;
  }
}

void Timescope::setParams(const avs::core::ParamBlock& params) {
  enabled_ = parseEnabled(params, enabled_);
  color_ = params.getInt("color", color_);
  sampleCount_ = parseSampleCount(params, sampleCount_);
  thickness_ = parseThickness(params, thickness_);
  channel_ = parseChannel(params, channel_);
  updateBlendModeFromParams(params);
}

int Timescope::nextCursor(int width) {
  if (width <= 0) {
    cursor_ = 0;
    lastWidth_ = -1;
    return 0;
  }
  if (width != lastWidth_) {
    cursor_ = 0;
    lastWidth_ = width;
    return cursor_;
  }
  cursor_ = (cursor_ + 1) % width;
  return cursor_;
}

void Timescope::applyBlend(std::uint8_t* pixel,
                           std::uint8_t r,
                           std::uint8_t g,
                           std::uint8_t b,
                           std::uint8_t a) const {
  switch (blendMode_) {
    case BlendMode::Replace:
      pixel[0] = r;
      pixel[1] = g;
      pixel[2] = b;
      pixel[3] = a;
      break;
    case BlendMode::Additive:
      pixel[0] = clampByte(pixel[0] + r);
      pixel[1] = clampByte(pixel[1] + g);
      pixel[2] = clampByte(pixel[2] + b);
      pixel[3] = clampByte(pixel[3] + a);
      break;
    case BlendMode::Average:
      pixel[0] = static_cast<std::uint8_t>((pixel[0] + r) / 2);
      pixel[1] = static_cast<std::uint8_t>((pixel[1] + g) / 2);
      pixel[2] = static_cast<std::uint8_t>((pixel[2] + b) / 2);
      pixel[3] = static_cast<std::uint8_t>((pixel[3] + a) / 2);
      break;
    case BlendMode::Max:
      pixel[0] = std::max(pixel[0], r);
      pixel[1] = std::max(pixel[1], g);
      pixel[2] = std::max(pixel[2], b);
      pixel[3] = std::max(pixel[3], a);
      break;
  }
}

bool Timescope::render(avs::core::RenderContext& context) {
  if (!enabled_) {
    return true;
  }
  if (!context.framebuffer.data || context.width <= 0 || context.height <= 0) {
    return true;
  }
  if (!context.audioAnalysis) {
    return true;
  }

  const auto& analysis = *context.audioAnalysis;
  const auto& waveform = analysis.waveform;
  const int width = context.width;
  const int height = context.height;
  const int stride = width * 4;
  const int x = nextCursor(width);

  const std::uint8_t baseR = static_cast<std::uint8_t>((color_ >> 16) & 0xFF);
  const std::uint8_t baseG = static_cast<std::uint8_t>((color_ >> 8) & 0xFF);
  const std::uint8_t baseB = static_cast<std::uint8_t>(color_ & 0xFF);

  const int samples = std::clamp(sampleCount_, kMinSampleCount, kWaveformSize);
  const float thickness = std::max(0.001f, thickness_);
  for (int y = 0; y < height; ++y) {
    std::uint8_t* pixel = context.framebuffer.data + y * stride + x * 4;
    const float level = (height > 1)
                            ? 1.0f - 2.0f * (static_cast<float>(y) / static_cast<float>(height - 1))
                            : 0.0f;
    float contribution = 0.0f;
    for (int i = 0; i < samples; ++i) {
      const int index = std::min(kWaveformSize - 1, (i * kWaveformSize) / samples);
      float sample = waveform[static_cast<std::size_t>(index)];
      (void)channel_;  // Analyzer currently provides mono waveform data.
      const float amplitude = std::abs(sample);
      const float distance = std::abs(sample - level);
      const float weight = std::max(0.0f, 1.0f - distance / thickness);
      contribution = std::max(contribution, weight * amplitude);
      if (contribution >= 1.0f) {
        break;
      }
    }

    const int intensity = static_cast<int>(std::round(std::clamp(contribution, 0.0f, 1.0f) * 255.0f));
    if (intensity == 0) {
      if (blendMode_ == BlendMode::Replace) {
        applyBlend(pixel, 0, 0, 0, 0);
      } else if (blendMode_ == BlendMode::Average) {
        applyBlend(pixel, 0, 0, 0, 0);
      }
      continue;
    }
    const std::uint8_t r = static_cast<std::uint8_t>((static_cast<int>(baseR) * intensity) / 255);
    const std::uint8_t g = static_cast<std::uint8_t>((static_cast<int>(baseG) * intensity) / 255);
    const std::uint8_t b = static_cast<std::uint8_t>((static_cast<int>(baseB) * intensity) / 255);
    const std::uint8_t a = intensity;
    applyBlend(pixel, r, g, b, a);
  }

  return true;
}

}  // namespace avs::effects::render

