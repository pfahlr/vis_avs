#include "avs/effects/Clear.hpp"

#include <algorithm>

namespace avs::effects {

bool Clear::render(avs::core::RenderContext& context) {
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
