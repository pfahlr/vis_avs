#include "effects/trans/effect_color_modifier.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <string>

namespace avs::effects::trans {

namespace {
constexpr std::size_t kBytesPerPixel = 4u;
constexpr float kMaxChannel = 255.0f;

float sineTransform(float value) {
  const float centered = value - 0.5f;
  return 0.5f * (std::sin(centered * std::numbers::pi_v<float>) + 1.0f);
}

float cosineTransform(float value) {
  const float centered = value - 0.5f;
  return 0.5f * (std::cos(centered * std::numbers::pi_v<float>) + 1.0f);
}

float phasedSine(float value, float phase) {
  const float tau = std::numbers::pi_v<float> * 2.0f;
  return 0.5f * (std::sin(value * tau + phase) + 1.0f);
}

}  // namespace

ColorModifier::ColorModifier() { updateLookupTables(); }

bool ColorModifier::render(avs::core::RenderContext& context) {
  if (context.framebuffer.data == nullptr || context.width <= 0 || context.height <= 0) {
    return true;
  }

  const std::size_t totalPixels =
      static_cast<std::size_t>(context.width) * static_cast<std::size_t>(context.height);
  const std::size_t requiredBytes = totalPixels * kBytesPerPixel;
  if (context.framebuffer.size < requiredBytes) {
    return true;
  }

  if (dirty_) {
    updateLookupTables();
  }

  std::uint8_t* pixels = context.framebuffer.data;
  for (std::size_t index = 0; index < totalPixels; ++index) {
    std::uint8_t* pixel = pixels + index * kBytesPerPixel;
    pixel[0] = redLut_[pixel[0]];
    pixel[1] = greenLut_[pixel[1]];
    pixel[2] = blueLut_[pixel[2]];
  }

  return true;
}

void ColorModifier::setParams(const avs::core::ParamBlock& params) {
  Mode newMode = mode_;

  std::string modeString = params.getString("mode", "");
  if (!modeString.empty()) {
    newMode = modeFromString(modeString, mode_);
  } else {
    newMode = modeFromInt(params.getInt("mode", static_cast<int>(mode_)));
  }

  if (newMode != mode_) {
    mode_ = newMode;
    dirty_ = true;
  }
}

void ColorModifier::updateLookupTables() {
  for (std::size_t value = 0; value < redLut_.size(); ++value) {
    const float normalized = static_cast<float>(value) / kMaxChannel;
    float red = normalized;
    float green = normalized;
    float blue = normalized;

    switch (mode_) {
      case Mode::Sine: {
        const float transformed = sineTransform(normalized);
        red = transformed;
        green = transformed;
        blue = transformed;
        break;
      }
      case Mode::Cosine: {
        const float transformed = cosineTransform(normalized);
        red = transformed;
        green = transformed;
        blue = transformed;
        break;
      }
      case Mode::SineCosine: {
        red = phasedSine(normalized, 0.0f);
        green = phasedSine(normalized, std::numbers::pi_v<float> * 2.0f / 3.0f);
        blue = phasedSine(normalized, std::numbers::pi_v<float> * 4.0f / 3.0f);
        break;
      }
    }

    redLut_[value] = static_cast<std::uint8_t>(std::lround(clamp01(red) * kMaxChannel));
    greenLut_[value] = static_cast<std::uint8_t>(std::lround(clamp01(green) * kMaxChannel));
    blueLut_[value] = static_cast<std::uint8_t>(std::lround(clamp01(blue) * kMaxChannel));
  }

  dirty_ = false;
}

float ColorModifier::clamp01(float value) { return std::clamp(value, 0.0f, 1.0f); }

ColorModifier::Mode ColorModifier::modeFromInt(int value) {
  switch (value) {
    case 1:
      return Mode::Cosine;
    case 2:
      return Mode::SineCosine;
    case 0:
    default:
      return Mode::Sine;
  }
}

ColorModifier::Mode ColorModifier::modeFromString(const std::string& value, Mode fallback) {
  std::string normalized;
  normalized.resize(value.size());
  std::transform(value.begin(), value.end(), normalized.begin(),
                 [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

  if (normalized == "sine" || normalized == "sin") {
    return Mode::Sine;
  }
  if (normalized == "cosine" || normalized == "cos") {
    return Mode::Cosine;
  }
  if (normalized == "sinecos" || normalized == "sine_cos" || normalized == "sinecosine" ||
      normalized == "sin_cos" || normalized == "mix") {
    return Mode::SineCosine;
  }

  return fallback;
}

}  // namespace avs::effects::trans
