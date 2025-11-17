#include "avs/effects/render/effect_onbeat_clear.h"

#include <cstring>

namespace avs::effects::render {

OnBeatClearEffect::OnBeatClearEffect() = default;

void OnBeatClearEffect::setParams(const avs::core::ParamBlock& params) {
  if (params.contains("color")) {
    color_ = static_cast<std::uint32_t>(params.getInt("color", static_cast<int>(color_)));
  }
  if (params.contains("enabled")) {
    enabled_ = params.getBool("enabled", enabled_);
  }
}

bool OnBeatClearEffect::render(avs::core::RenderContext& context) {
  if (!enabled_ || !context.audioBeat) {
    return true;
  }

  // Beat detected - clear the framebuffer
  const std::size_t pixelCount = context.width * context.height;
  std::uint8_t* data = context.framebuffer.data;

  const std::uint8_t r = (color_ >> 16) & 0xFF;
  const std::uint8_t g = (color_ >> 8) & 0xFF;
  const std::uint8_t b = color_ & 0xFF;

  for (std::size_t i = 0; i < pixelCount; ++i) {
    const std::size_t offset = i * 4;
    data[offset + 0] = r;
    data[offset + 1] = g;
    data[offset + 2] = b;
    data[offset + 3] = 255;
  }

  return true;
}

}  // namespace avs::effects::render
