#include "avs/effects/trans/effect_invert.h"

#include <avs/core/IFramebuffer.hpp>

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

  // Modern path: use framebuffer backend if available
  std::uint8_t* data = nullptr;
  std::size_t pixelCount = 0;

  if (context.framebufferBackend) {
    data = context.framebufferBackend->data();
    pixelCount = static_cast<std::size_t>(context.framebufferBackend->width()) *
                 context.framebufferBackend->height();
  } else {
    // Legacy path: direct pixel buffer access
    data = context.framebuffer.data;
    pixelCount = static_cast<std::size_t>(context.width) * context.height;
  }

  // Invert RGB channels
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
