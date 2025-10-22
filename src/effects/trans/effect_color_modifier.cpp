#include "effects/trans/effect_color_modifier.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>
#include <string>
#include <string_view>

namespace {

bool hasFramebuffer(const avs::core::RenderContext& context) {
  return context.framebuffer.data != nullptr && context.framebuffer.size >= 4u &&
         context.width > 0 && context.height > 0;
}

float clampNormalized(float value) { return std::clamp(value, 0.0f, 1.0f); }

std::uint8_t toByte(float normalized) {
  const float clamped = clampNormalized(normalized);
  const int scaled = static_cast<int>(std::lround(clamped * 255.0f));
  return static_cast<std::uint8_t>(std::clamp(scaled, 0, 255));
}

using Mode = avs::effects::trans::ColorModifier::Mode;

Mode parseModeString(std::string value, Mode fallback) {
  if (value.empty()) {
    return fallback;
  }

  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

  value.erase(std::remove_if(value.begin(), value.end(),
                             [](unsigned char c) { return c == ' ' || c == '-' || c == '_'; }),
              value.end());

  if (value == "identity" || value == "none") {
    return Mode::Identity;
  }
  if (value == "sine" || value == "sin") {
    return Mode::Sine;
  }
  if (value == "cosine" || value == "cos") {
    return Mode::Cosine;
  }
  if (value == "sinecosine" || value == "sincos" || value == "sinuscosinus") {
    return Mode::SineCosine;
  }
  return fallback;
}

Mode parseModeInt(int value, Mode fallback) {
  switch (value) {
    case 0:
      return Mode::Identity;
    case 1:
      return Mode::Sine;
    case 2:
      return Mode::Cosine;
    case 3:
      return Mode::SineCosine;
    default:
      return fallback;
  }
}

Mode readModeParam(const avs::core::ParamBlock& params, Mode fallback) {
  constexpr std::array<std::string_view, 4> kKeys{"mode", "modifier_mode", "color_modifier_mode",
                                                  "color_mode"};
  for (const auto& keyView : kKeys) {
    const std::string key(keyView);
    if (!params.contains(key)) {
      continue;
    }

    const std::string sentinel(1, '\x01');
    const std::string asString = params.getString(key, sentinel);
    if (asString != sentinel) {
      return parseModeString(asString, fallback);
    }

    const int asInt = params.getInt(key, static_cast<int>(fallback));
    return parseModeInt(asInt, fallback);
  }
  return fallback;
}

bool readBoolParam(const avs::core::ParamBlock& params, const std::array<std::string_view, 3>& keys,
                   bool fallback) {
  for (const auto& keyView : keys) {
    const std::string key(keyView);
    if (!params.contains(key)) {
      continue;
    }
    bool value = params.getBool(key, fallback);
    if (!value) {
      value = params.getInt(key, value ? 1 : 0) != 0;
    }
    return value;
  }
  return fallback;
}

std::array<float, 3> evaluateNormalized(float normalized, Mode mode) {
  constexpr float kPi = std::numbers::pi_v<float>;
  const float centered = normalized - 0.5f;
  const float sine = std::sin(centered * kPi);
  const float cosine = std::cos(centered * kPi);
  const float sineNormalized = 0.5f * (sine + 1.0f);
  const float cosineNormalized = 0.5f * (cosine + 1.0f);

  switch (mode) {
    case Mode::Identity:
      return {normalized, normalized, normalized};
    case Mode::Sine:
      return {sineNormalized, sineNormalized, sineNormalized};
    case Mode::Cosine:
      return {cosineNormalized, cosineNormalized, cosineNormalized};
    case Mode::SineCosine: {
      const float mix = 0.5f * (sineNormalized + cosineNormalized);
      return {sineNormalized, cosineNormalized, mix};
    }
  }
  return {normalized, normalized, normalized};
}

}  // namespace

namespace avs::effects::trans {

void ColorModifier::setParams(const avs::core::ParamBlock& params) {
  const bool newEnabled = readBoolParam(params, {"enabled", "active", "on"}, enabled_);
  const Mode newMode = readModeParam(params, mode_);

  if (newMode != mode_) {
    tablesDirty_ = true;
  }

  enabled_ = newEnabled;
  mode_ = newMode;
}

bool ColorModifier::render(avs::core::RenderContext& context) {
  if (!enabled_) {
    return true;
  }
  if (!hasFramebuffer(context)) {
    return true;
  }

  recomputeLookupTables();

  std::uint8_t* pixels = context.framebuffer.data;
  const std::size_t totalPixels =
      static_cast<std::size_t>(context.width) * static_cast<std::size_t>(context.height);
  for (std::size_t i = 0; i < totalPixels; ++i) {
    std::uint8_t* px = pixels + i * 4u;
    px[0] = redTable_[px[0]];
    px[1] = greenTable_[px[1]];
    px[2] = blueTable_[px[2]];
  }

  return true;
}

void ColorModifier::recomputeLookupTables() {
  if (!tablesDirty_) {
    return;
  }

  for (int value = 0; value < 256; ++value) {
    const float normalized = static_cast<float>(value) / 255.0f;
    const auto outputs = evaluateNormalized(normalized, mode_);
    redTable_[value] = toByte(outputs[0]);
    greenTable_[value] = toByte(outputs[1]);
    blueTable_[value] = toByte(outputs[2]);
  }

  tablesDirty_ = false;
}

}  // namespace avs::effects::trans
