#include <avs/effects/prime/Clear.hpp>

#include <avs/core/IFramebuffer.hpp>
#include <algorithm>

namespace avs::effects {

bool Clear::render(avs::core::RenderContext& context) {
  // Modern path: use framebuffer backend if available
  if (context.framebufferBackend) {
    // Clear all channels (RGBA) to the same value to match legacy behavior
    // Legacy Clear fills every byte with value_, including alpha
    context.framebufferBackend->clear(value_, value_, value_, value_);
    return true;
  }

  // Legacy path: direct pixel buffer access
  if (!context.framebuffer.data) {
    return true;
  }
  std::fill(context.framebuffer.data,
            context.framebuffer.data + context.framebuffer.size, value_);
  return true;
}

void Clear::setParams(const avs::core::ParamBlock& params) {
  value_ = static_cast<std::uint8_t>(params.getInt("value", value_));
}

}  // namespace avs::effects
