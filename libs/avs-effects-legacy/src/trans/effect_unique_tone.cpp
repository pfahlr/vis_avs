#include <avs/effects/trans/effect_unique_tone.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>

namespace {

bool hasFramebuffer(const avs::core::RenderContext& context) {
  return context.framebuffer.data != nullptr && context.framebuffer.size >= 4u && context.width > 0 &&
         context.height > 0;
}

bool readFlag(const avs::core::ParamBlock& params,
              std::initializer_list<const char*> keys,
              bool fallback) {
  for (const char* key : keys) {
    if (!params.contains(key)) {
      continue;
    }
    bool value = params.getBool(key, fallback);
    int valueInt = params.getInt(key, value ? 1 : 0);
    return valueInt != 0;
  }
  return fallback;
}

std::uint32_t readColor(const avs::core::ParamBlock& params,
                        std::initializer_list<const char*> keys,
                        std::uint32_t fallback) {
  for (const char* key : keys) {
    if (!params.contains(key)) {
      continue;
    }
    return static_cast<std::uint32_t>(params.getInt(key, static_cast<int>(fallback)));
  }
  return fallback;
}

}  // namespace

namespace avs::effects::trans {

UniqueTone::UniqueTone() { rebuildLookupTables(); }

void UniqueTone::setParams(const avs::core::ParamBlock& params) {
  enabled_ = readFlag(params, {"enabled", "active", "on"}, enabled_);
  invert_ = readFlag(params, {"invert", "invert_luminance", "negative"}, invert_);

  const bool additive = readFlag(params, {"blend", "additive", "blend_additive"},
                                 blendMode_ == BlendMode::kAdditive);
  const bool average = readFlag(params, {"blendavg", "blend_average", "average"},
                                blendMode_ == BlendMode::kAverage);

  if (additive) {
    blendMode_ = BlendMode::kAdditive;
  } else if (average) {
    blendMode_ = BlendMode::kAverage;
  } else {
    blendMode_ = BlendMode::kReplace;
  }

  const std::uint32_t newColor = readColor(params,
                                           {"color", "tone_color", "unique_tone_color", "unique_color"},
                                           color_);
  if (newColor != color_) {
    color_ = newColor & 0x00FFFFFFu;
    toneColor_[0] = static_cast<std::uint8_t>((color_ >> 16) & 0xFFu);
    toneColor_[1] = static_cast<std::uint8_t>((color_ >> 8) & 0xFFu);
    toneColor_[2] = static_cast<std::uint8_t>(color_ & 0xFFu);
    tablesDirty_ = true;
  }
}

bool UniqueTone::render(avs::core::RenderContext& context) {
  if (!enabled_ || !hasFramebuffer(context)) {
    return true;
  }

  rebuildLookupTables();

  std::uint8_t* pixels = context.framebuffer.data;
  const std::size_t totalPixels = static_cast<std::size_t>(context.width) * static_cast<std::size_t>(context.height);
  const std::size_t availablePixels = context.framebuffer.size / 4u;
  const std::size_t pixelCount = std::min(totalPixels, availablePixels);

  if (pixelCount == 0) {
    return true;
  }

  for (std::size_t i = 0; i < pixelCount; ++i) {
    std::uint8_t* px = pixels + i * 4u;
    int depth = px[0];
    if (px[1] > depth) {
      depth = px[1];
    }
    if (px[2] > depth) {
      depth = px[2];
    }
    depth = invert_ ? 255 - depth : depth;
    if (depth < 0) {
      depth = 0;
    } else if (depth > 255) {
      depth = 255;
    }

    const std::uint8_t red = redTable_[static_cast<std::size_t>(depth)];
    const std::uint8_t green = greenTable_[static_cast<std::size_t>(depth)];
    const std::uint8_t blue = blueTable_[static_cast<std::size_t>(depth)];

    switch (blendMode_) {
      case BlendMode::kAdditive:
        px[0] = saturatingAdd(px[0], red);
        px[1] = saturatingAdd(px[1], green);
        px[2] = saturatingAdd(px[2], blue);
        break;
      case BlendMode::kAverage:
        px[0] = static_cast<std::uint8_t>((static_cast<int>(px[0]) + static_cast<int>(red)) >> 1);
        px[1] = static_cast<std::uint8_t>((static_cast<int>(px[1]) + static_cast<int>(green)) >> 1);
        px[2] = static_cast<std::uint8_t>((static_cast<int>(px[2]) + static_cast<int>(blue)) >> 1);
        break;
      case BlendMode::kReplace:
      default:
        px[0] = red;
        px[1] = green;
        px[2] = blue;
        break;
    }
  }

  return true;
}

void UniqueTone::rebuildLookupTables() {
  if (!tablesDirty_) {
    return;
  }

  for (int i = 0; i < 256; ++i) {
    const auto intensity = static_cast<std::uint32_t>(i);
    redTable_[static_cast<std::size_t>(i)] =
        static_cast<std::uint8_t>((intensity * static_cast<std::uint32_t>(toneColor_[0])) / 255u);
    greenTable_[static_cast<std::size_t>(i)] =
        static_cast<std::uint8_t>((intensity * static_cast<std::uint32_t>(toneColor_[1])) / 255u);
    blueTable_[static_cast<std::size_t>(i)] =
        static_cast<std::uint8_t>((intensity * static_cast<std::uint32_t>(toneColor_[2])) / 255u);
  }

  tablesDirty_ = false;
}

bool UniqueTone::hasFramebuffer(const avs::core::RenderContext& context) {
  return ::hasFramebuffer(context);
}

std::uint8_t UniqueTone::saturatingAdd(std::uint8_t base, std::uint8_t addend) {
  const int sum = static_cast<int>(base) + static_cast<int>(addend);
  return static_cast<std::uint8_t>(sum > 255 ? 255 : sum);
}

}  // namespace avs::effects::trans
