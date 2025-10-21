#include "effects/trans/effect_unique_tone.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <string>

namespace {

bool hasFramebuffer(const avs::core::RenderContext& context) {
  return context.framebuffer.data != nullptr && context.framebuffer.size >= 4u &&
         context.width > 0 && context.height > 0;
}

int clampComponent(int value) { return value < 0 ? 0 : (value > 255 ? 255 : value); }

std::string toLower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  return value;
}

}  // namespace

namespace avs::effects::trans {

namespace {
constexpr float kRedWeight = 0.2126f;
constexpr float kGreenWeight = 0.7152f;
constexpr float kBlueWeight = 0.0722f;
}  // namespace

UniqueTone::UniqueTone() { rebuildToneScale(); }

bool UniqueTone::render(avs::core::RenderContext& context) {
  if (!enabled_ || !hasFramebuffer(context)) {
    return true;
  }

  std::uint8_t* pixels = context.framebuffer.data;
  const std::size_t totalPixels =
      static_cast<std::size_t>(context.width) * static_cast<std::size_t>(context.height);
  for (std::size_t index = 0; index < totalPixels; ++index) {
    std::uint8_t* pixel = pixels + index * 4u;
    const float red = static_cast<float>(pixel[0]);
    const float green = static_cast<float>(pixel[1]);
    const float blue = static_cast<float>(pixel[2]);

    float luminance = computeLuminance(red, green, blue);
    if (invert_) {
      luminance = 255.0f - luminance;
    }
    luminance = std::clamp(luminance, 0.0f, 255.0f);

    const std::uint8_t toneR = toByte(toneScale_[0] * luminance);
    const std::uint8_t toneG = toByte(toneScale_[1] * luminance);
    const std::uint8_t toneB = toByte(toneScale_[2] * luminance);

    switch (blendMode_) {
      case BlendMode::Replace:
        pixel[0] = toneR;
        pixel[1] = toneG;
        pixel[2] = toneB;
        break;
      case BlendMode::Additive:
        pixel[0] = saturatingAdd(pixel[0], toneR);
        pixel[1] = saturatingAdd(pixel[1], toneG);
        pixel[2] = saturatingAdd(pixel[2], toneB);
        break;
      case BlendMode::Average:
        pixel[0] = blendAverage(pixel[0], toneR);
        pixel[1] = blendAverage(pixel[1], toneG);
        pixel[2] = blendAverage(pixel[2], toneB);
        break;
    }
  }

  return true;
}

void UniqueTone::setParams(const avs::core::ParamBlock& params) {
  if (params.contains("enabled")) {
    enabled_ = params.getBool("enabled", enabled_);
  }
  invert_ = params.getBool("invert", invert_);

  BlendMode newMode = blendMode_;
  if (params.contains("blend_mode")) {
    newMode = parseBlendModeValue(params.getString("blend_mode"), newMode);
  }
  if (params.contains("mode")) {
    newMode = parseBlendModeValue(params.getString("mode"), newMode);
  }

  const bool blendFlag = params.getBool("blend", false);
  const bool blendAvgFlag = params.getBool("blendavg", false);
  const bool replaceFlag = params.getBool("replace", false);
  if (params.contains("blend") || params.contains("blendavg") || params.contains("replace")) {
    if (blendAvgFlag) {
      newMode = BlendMode::Average;
    } else if (blendFlag) {
      newMode = BlendMode::Additive;
    } else if (replaceFlag) {
      newMode = BlendMode::Replace;
    } else {
      newMode = BlendMode::Replace;
    }
  }
  blendMode_ = newMode;

  bool colorChanged = false;
  int candidate = color_;
  if (params.contains("color")) {
    int raw = params.getInt("color", candidate);
    if (raw < 0) {
      raw = 0;
    }
    if (raw > 0xFFFFFF) {
      raw = 0xFFFFFF;
    }
    candidate = raw;
    colorChanged = true;
  }

  if (params.contains("color_r") || params.contains("color_g") || params.contains("color_b")) {
    int red = params.getInt("color_r", (candidate >> 16) & 0xFF);
    int green = params.getInt("color_g", (candidate >> 8) & 0xFF);
    int blue = params.getInt("color_b", candidate & 0xFF);
    red = clampComponent(red);
    green = clampComponent(green);
    blue = clampComponent(blue);
    candidate = (red << 16) | (green << 8) | blue;
    colorChanged = true;
  }

  if (colorChanged && candidate != color_) {
    color_ = candidate;
    rebuildToneScale();
  } else if (!colorChanged && toneLuminance_ == 0.0f) {
    rebuildToneScale();
  }
}

void UniqueTone::rebuildToneScale() {
  const float red = static_cast<float>((color_ >> 16) & 0xFF);
  const float green = static_cast<float>((color_ >> 8) & 0xFF);
  const float blue = static_cast<float>(color_ & 0xFF);
  toneLuminance_ = computeLuminance(red, green, blue);
  if (toneLuminance_ > 1e-5f) {
    toneScale_[0] = red / toneLuminance_;
    toneScale_[1] = green / toneLuminance_;
    toneScale_[2] = blue / toneLuminance_;
  } else {
    toneLuminance_ = 0.0f;
    toneScale_.fill(0.0f);
  }
}

float UniqueTone::computeLuminance(float red, float green, float blue) {
  return red * kRedWeight + green * kGreenWeight + blue * kBlueWeight;
}

std::uint8_t UniqueTone::toByte(float value) {
  const float clamped = std::clamp(value, 0.0f, 255.0f);
  const auto rounded = static_cast<int>(std::lround(clamped));
  if (rounded <= 0) {
    return 0u;
  }
  if (rounded >= 255) {
    return 255u;
  }
  return static_cast<std::uint8_t>(rounded);
}

std::uint8_t UniqueTone::saturatingAdd(std::uint8_t base, std::uint8_t overlay) {
  const int sum = static_cast<int>(base) + static_cast<int>(overlay);
  return static_cast<std::uint8_t>(sum >= 255 ? 255 : sum);
}

std::uint8_t UniqueTone::blendAverage(std::uint8_t a, std::uint8_t b) {
  return static_cast<std::uint8_t>((static_cast<int>(a) + static_cast<int>(b)) / 2);
}

UniqueTone::BlendMode UniqueTone::parseBlendModeValue(const std::string& value,
                                                      BlendMode fallback) {
  const std::string lower = toLower(value);
  if (lower == "add" || lower == "additive" || lower == "blend") {
    return BlendMode::Additive;
  }
  if (lower == "avg" || lower == "average" || lower == "5050" || lower == "half") {
    return BlendMode::Average;
  }
  if (lower == "replace" || lower == "copy" || lower == "normal") {
    return BlendMode::Replace;
  }
  return fallback;
}

}  // namespace avs::effects::trans
