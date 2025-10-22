#include <avs/effects/legacy/trans/effect_brightness.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>

namespace {

inline bool hasFramebuffer(const avs::core::RenderContext& context) {
  return context.framebuffer.data != nullptr && context.framebuffer.size >= 4u &&
         context.width > 0 && context.height > 0;
}

}  // namespace

namespace avs::effects::trans {

bool Brightness::render(avs::core::RenderContext& context) {
  if (!enabled_) {
    return true;
  }
  if (!hasFramebuffer(context)) {
    return true;
  }

  updateLookupTables();

  const bool useAdditiveBlend = blendAdditive_;
  const bool useAverageBlend = !useAdditiveBlend && blendAverage_;

  std::uint8_t* pixels = context.framebuffer.data;
  const std::size_t totalPixels =
      static_cast<std::size_t>(context.width) * static_cast<std::size_t>(context.height);
  for (std::size_t i = 0; i < totalPixels; ++i) {
    std::uint8_t* px = pixels + i * 4u;
    if (exclude_ && shouldSkipPixel(px)) {
      continue;
    }

    const std::uint8_t newRed = redTable_[px[0]];
    const std::uint8_t newGreen = greenTable_[px[1]];
    const std::uint8_t newBlue = blueTable_[px[2]];

    if (useAdditiveBlend) {
      px[0] = saturatingAdd(px[0], newRed);
      px[1] = saturatingAdd(px[1], newGreen);
      px[2] = saturatingAdd(px[2], newBlue);
    } else if (useAverageBlend) {
      px[0] = static_cast<std::uint8_t>((static_cast<int>(px[0]) + static_cast<int>(newRed)) >> 1);
      px[1] =
          static_cast<std::uint8_t>((static_cast<int>(px[1]) + static_cast<int>(newGreen)) >> 1);
      px[2] = static_cast<std::uint8_t>((static_cast<int>(px[2]) + static_cast<int>(newBlue)) >> 1);
    } else {
      px[0] = newRed;
      px[1] = newGreen;
      px[2] = newBlue;
    }
  }

  return true;
}

void Brightness::setParams(const avs::core::ParamBlock& params) {
  auto readBool = [&params](const std::string& key, bool current) {
    bool value = params.getBool(key, current);
    value = params.getInt(key, value ? 1 : 0) != 0;
    return value;
  };

  const bool newEnabled = readBool("enabled", enabled_);
  const bool newBlendAdditive = readBool("blend", blendAdditive_);
  const bool newBlendAverage = readBool("blendavg", blendAverage_);
  const bool newExclude = readBool("exclude", exclude_);

  const int newDistance = std::clamp(params.getInt("distance", distance_), 0, 255);
  const std::uint32_t newColor =
      static_cast<std::uint32_t>(params.getInt("color", static_cast<int>(referenceColor_)));
  const int newRedSlider = params.getInt("redp", redSlider_);
  const int newGreenSlider = params.getInt("greenp", greenSlider_);
  const int newBlueSlider = params.getInt("bluep", blueSlider_);

  if (newRedSlider != redSlider_ || newGreenSlider != greenSlider_ ||
      newBlueSlider != blueSlider_) {
    tablesDirty_ = true;
  }

  enabled_ = newEnabled;
  blendAdditive_ = newBlendAdditive;
  blendAverage_ = newBlendAverage;
  exclude_ = newExclude;
  distance_ = newDistance;
  referenceColor_ = newColor;
  redSlider_ = newRedSlider;
  greenSlider_ = newGreenSlider;
  blueSlider_ = newBlueSlider;

  referenceRed_ = static_cast<std::uint8_t>((referenceColor_ >> 16) & 0xFFu);
  referenceGreen_ = static_cast<std::uint8_t>((referenceColor_ >> 8) & 0xFFu);
  referenceBlue_ = static_cast<std::uint8_t>(referenceColor_ & 0xFFu);
}

void Brightness::updateLookupTables() {
  if (!tablesDirty_) {
    return;
  }

  const int redMultiplier = computeMultiplier(redSlider_);
  const int greenMultiplier = computeMultiplier(greenSlider_);
  const int blueMultiplier = computeMultiplier(blueSlider_);

  for (int value = 0; value < 256; ++value) {
    const std::uint8_t base = static_cast<std::uint8_t>(value);
    redTable_[value] = applyMultiplier(base, redMultiplier);
    greenTable_[value] = applyMultiplier(base, greenMultiplier);
    blueTable_[value] = applyMultiplier(base, blueMultiplier);
  }

  tablesDirty_ = false;
}

bool Brightness::shouldSkipPixel(const std::uint8_t* pixel) const {
  if (!exclude_) {
    return false;
  }
  const int dr = std::abs(static_cast<int>(pixel[0]) - static_cast<int>(referenceRed_));
  const int dg = std::abs(static_cast<int>(pixel[1]) - static_cast<int>(referenceGreen_));
  const int db = std::abs(static_cast<int>(pixel[2]) - static_cast<int>(referenceBlue_));
  return dr <= distance_ && dg <= distance_ && db <= distance_;
}

int Brightness::computeMultiplier(int sliderValue) {
  const float scale = sliderValue < 0 ? 1.0f : 16.0f;
  const float factor = 1.0f + scale * static_cast<float>(sliderValue) / 4096.0f;
  const float clampedFactor = std::max(factor, 0.0f);
  return static_cast<int>(clampedFactor * 65536.0f);
}

std::uint8_t Brightness::applyMultiplier(std::uint8_t value, int multiplier) {
  const std::int64_t scaled =
      (static_cast<std::int64_t>(value) * static_cast<std::int64_t>(multiplier)) >> 16;
  int result = static_cast<int>(scaled);
  if (result < 0) {
    result = 0;
  } else if (result > 255) {
    result = 255;
  }
  return static_cast<std::uint8_t>(result);
}

std::uint8_t Brightness::saturatingAdd(std::uint8_t base, std::uint8_t addend) {
  const int sum = static_cast<int>(base) + static_cast<int>(addend);
  return static_cast<std::uint8_t>(sum > 255 ? 255 : sum);
}

}  // namespace avs::effects::trans
