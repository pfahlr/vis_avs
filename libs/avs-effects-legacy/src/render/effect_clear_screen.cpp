#include "avs/effects/render/effect_clear_screen.h"

#include <algorithm>
#include <cstring>

namespace avs::effects::render {

ClearScreenEffect::ClearScreenEffect() = default;

void ClearScreenEffect::setParams(const avs::core::ParamBlock& params) {
  if (params.contains("color")) {
    color_ = static_cast<std::uint32_t>(params.getInt("color", static_cast<int>(color_)));
  }
  if (params.contains("blend_mode")) {
    blendMode_ = params.getInt("blend_mode", blendMode_);
  }
}

bool ClearScreenEffect::render(avs::core::RenderContext& context) {
  const std::size_t pixelCount = context.width * context.height;
  std::uint8_t* data = context.framebuffer.data;

  const std::uint8_t r = (color_ >> 16) & 0xFF;
  const std::uint8_t g = (color_ >> 8) & 0xFF;
  const std::uint8_t b = color_ & 0xFF;

  if (blendMode_ == 0) {
    // Replace mode - simple fill
    for (std::size_t i = 0; i < pixelCount; ++i) {
      const std::size_t offset = i * 4;
      data[offset + 0] = r;
      data[offset + 1] = g;
      data[offset + 2] = b;
      data[offset + 3] = 255;
    }
  } else if (blendMode_ == 1) {
    // Additive blend
    for (std::size_t i = 0; i < pixelCount; ++i) {
      const std::size_t offset = i * 4;
      data[offset + 0] = std::min(255, data[offset + 0] + r);
      data[offset + 1] = std::min(255, data[offset + 1] + g);
      data[offset + 2] = std::min(255, data[offset + 2] + b);
    }
  } else {
    // TODO: Implement other blend modes (maximum, average, etc.)
    // For now, default to replace
    for (std::size_t i = 0; i < pixelCount; ++i) {
      const std::size_t offset = i * 4;
      data[offset + 0] = r;
      data[offset + 1] = g;
      data[offset + 2] = b;
      data[offset + 3] = 255;
    }
  }

  return true;
}

}  // namespace avs::effects::render
