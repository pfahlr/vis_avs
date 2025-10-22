#include <avs/effects/legacy/trans/effect_color_clip.h>

#include <array>
#include <cstddef>

namespace avs::effects::trans {
namespace {
bool hasFramebuffer(const avs::core::RenderContext& context) {
  return context.framebuffer.data != nullptr && context.framebuffer.size >= 4u && context.width > 0 &&
         context.height > 0;
}

std::uint8_t extractComponent(int value, int shift) {
  return static_cast<std::uint8_t>((value >> shift) & 0xFF);
}

int composeColor(const std::array<std::uint8_t, 3>& color) {
  return static_cast<int>(color[0]) | (static_cast<int>(color[1]) << 8) |
         (static_cast<int>(color[2]) << 16);
}

int readColorParam(const avs::core::ParamBlock& params, int fallback) {
  if (params.contains("color")) {
    return params.getInt("color", fallback);
  }
  if (params.contains("color_clip")) {
    return params.getInt("color_clip", fallback);
  }
  if (params.contains("clip_color")) {
    return params.getInt("clip_color", fallback);
  }
  return fallback;
}

bool readEnabled(const avs::core::ParamBlock& params, bool fallback) {
  if (params.contains("enabled")) {
    return params.getBool("enabled", fallback);
  }
  if (params.contains("active")) {
    return params.getBool("active", fallback);
  }
  if (params.contains("on")) {
    return params.getBool("on", fallback);
  }
  return fallback;
}

}  // namespace

void ColorClip::setParams(const avs::core::ParamBlock& params) {
  enabled_ = readEnabled(params, enabled_);
  const int defaultColor = composeColor(clipColor_);
  const int colorValue = readColorParam(params, defaultColor);
  clipColor_[0] = extractComponent(colorValue, 0);
  clipColor_[1] = extractComponent(colorValue, 8);
  clipColor_[2] = extractComponent(colorValue, 16);
}

bool ColorClip::render(avs::core::RenderContext& context) {
  if (!enabled_) {
    return true;
  }
  if (!hasFramebuffer(context)) {
    return true;
  }

  std::uint8_t* pixels = context.framebuffer.data;
  const std::size_t totalPixels = static_cast<std::size_t>(context.width) * static_cast<std::size_t>(context.height);
  for (std::size_t i = 0; i < totalPixels; ++i) {
    std::uint8_t* px = pixels + i * 4u;
    if (px[0] <= clipColor_[0] && px[1] <= clipColor_[1] && px[2] <= clipColor_[2]) {
      px[0] = clipColor_[0];
      px[1] = clipColor_[1];
      px[2] = clipColor_[2];
    }
  }
  return true;
}

}  // namespace avs::effects::trans

