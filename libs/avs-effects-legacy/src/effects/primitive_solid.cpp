#include <avs/effects/Primitives.hpp>

#include <algorithm>

#include "primitive_common.hpp"

namespace avs::effects {

void PrimitiveSolid::setParams(const avs::core::ParamBlock& params) {
  x1_ = params.getInt("x1", params.getInt("left", x1_));
  y1_ = params.getInt("y1", params.getInt("top", y1_));
  x2_ = params.getInt("x2", params.getInt("right", x2_));
  y2_ = params.getInt("y2", params.getInt("bottom", y2_));
  color_ = params.getInt("color", color_);
  alpha_ = params.getInt("alpha", alpha_);
}

bool PrimitiveSolid::render(avs::core::RenderContext& context) {
  if (!context.framebuffer.data || context.width <= 0 || context.height <= 0) {
    return true;
  }
  int minX = std::min(x1_, x2_);
  int maxX = std::max(x1_, x2_);
  int minY = std::min(y1_, y2_);
  int maxY = std::max(y1_, y2_);
  minX = std::max(0, minX);
  minY = std::max(0, minY);
  maxX = std::min(context.width - 1, maxX);
  maxY = std::min(context.height - 1, maxY);
  detail::RGBA color = detail::colorFromInt(color_, detail::clampByte(alpha_));
  for (int y = minY; y <= maxY; ++y) {
    for (int x = minX; x <= maxX; ++x) {
      detail::blendPixel(context, x, y, color);
    }
  }
  return true;
}

}  // namespace avs::effects

