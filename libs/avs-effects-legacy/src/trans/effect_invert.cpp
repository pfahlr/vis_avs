#include "avs/effects/trans/effect_invert.h"

namespace avs::effects::trans {

void InvertEffect::setParams(const avs::core::ParamBlock& params) {
  if (params.contains("enabled")) {
    enabled_ = params.getBool("enabled", enabled_);
  }
}

bool InvertEffect::render(avs::core::RenderContext& context) {
  if (!enabled_) {
    return true;
  }

  const std::size_t pixelCount = context.width * context.height;
  std::uint8_t* data = context.framebuffer.data;

  for (std::size_t i = 0; i < pixelCount; ++i) {
    const std::size_t offset = i * 4;
    // Invert RGB, leave alpha unchanged
    data[offset + 0] = 255 - data[offset + 0];  // R
    data[offset + 1] = 255 - data[offset + 1];  // G
    data[offset + 2] = 255 - data[offset + 2];  // B
    // data[offset + 3] unchanged (A)
  }

  return true;
}

}  // namespace avs::effects::trans
