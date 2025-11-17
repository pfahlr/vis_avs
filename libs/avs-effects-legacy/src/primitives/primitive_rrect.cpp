#include <avs/effects/primitives/Primitives.hpp>

#include <algorithm>

#include <avs/effects/primitives/primitive_common.hpp>

namespace avs::effects {

void PrimitiveRoundedRect::setParams(const avs::core::ParamBlock& params) {
  x_ = params.getInt("x", x_);
  y_ = params.getInt("y", y_);
  width_ = params.getInt("width", width_);
  height_ = params.getInt("height", height_);
  if (width_ <= 0 && params.contains("x2")) {
    width_ = params.getInt("x2") - x_ + 1;
  }
  if (height_ <= 0 && params.contains("y2")) {
    height_ = params.getInt("y2") - y_ + 1;
  }
  radius_ = std::max(0, params.getInt("radius", params.getInt("round", radius_)));
  filled_ = params.getBool("filled", filled_);
  color_ = params.getInt("color", color_);
  alpha_ = params.getInt("alpha", alpha_);
  outlineColor_ = params.getInt("outlinecolor", outlineColor_);
  outlineAlpha_ = params.getInt("outlinealpha", outlineAlpha_);
  outlineWidth_ = std::max(0, params.getInt("outlinesize", params.getInt("outlinewidth", outlineWidth_)));
}

static bool containsPoint(int px,
                          int py,
                          int x0,
                          int y0,
                          int x1,
                          int y1,
                          int radius,
                          int shrink) {
  x0 += shrink;
  y0 += shrink;
  x1 -= shrink;
  y1 -= shrink;
  if (x0 > x1 || y0 > y1) {
    return false;
  }
  radius = std::max(0, radius - shrink);
  if (px < x0 || px > x1 || py < y0 || py > y1) {
    return false;
  }
  if (radius == 0) {
    return true;
  }
  if ((px >= x0 + radius && px <= x1 - radius) || (py >= y0 + radius && py <= y1 - radius)) {
    return true;
  }
  const int cx = (px < x0 + radius) ? x0 + radius : x1 - radius;
  const int cy = (py < y0 + radius) ? y0 + radius : y1 - radius;
  const int dx = px - cx;
  const int dy = py - cy;
  return dx * dx + dy * dy <= radius * radius;
}

bool PrimitiveRoundedRect::render(avs::core::RenderContext& context) {
  if (!context.framebuffer.data || context.width <= 0 || context.height <= 0) {
    return true;
  }
  if (width_ <= 0 || height_ <= 0) {
    return true;
  }
  const int x0 = x_;
  const int y0 = y_;
  const int x1 = x_ + width_ - 1;
  const int y1 = y_ + height_ - 1;
  const int radius = std::min({radius_, (x1 - x0) / 2, (y1 - y0) / 2});
  const detail::RGBA fillColor = detail::colorFromInt(color_, detail::clampByte(alpha_));
  const detail::RGBA outlineColor = detail::colorFromInt(outlineColor_, detail::clampByte(outlineAlpha_));
  for (int y = std::max(0, y0); y <= std::min(context.height - 1, y1); ++y) {
    for (int x = std::max(0, x0); x <= std::min(context.width - 1, x1); ++x) {
      const bool inside = containsPoint(x, y, x0, y0, x1, y1, radius, 0);
      if (!inside) {
        continue;
      }
      bool drawOutline = false;
      if (outlineWidth_ > 0) {
        drawOutline = !containsPoint(x, y, x0, y0, x1, y1, radius, outlineWidth_);
      }
      if (drawOutline) {
        detail::blendPixel(context, x, y, outlineColor);
      } else if (filled_) {
        detail::blendPixel(context, x, y, fillColor);
      }
    }
  }
  return true;
}

}  // namespace avs::effects

