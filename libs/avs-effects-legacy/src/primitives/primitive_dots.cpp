#include <avs/effects/Primitives.hpp>

#include <algorithm>

#include <avs/effects/legacy/primitives/primitive_common.hpp>

namespace avs::effects {

void PrimitiveDots::setParams(const avs::core::ParamBlock& params) {
  points_.clear();
  const std::string list = params.getString("points", "");
  if (!list.empty()) {
    auto parsed = detail::parsePointList(list);
    points_.reserve(parsed.size());
    for (const auto& p : parsed) {
      points_.emplace_back(p.x, p.y);
    }
  }
  if (points_.empty()) {
    const bool hasX = params.contains("x");
    const bool hasY = params.contains("y");
    if (hasX && hasY) {
      points_.emplace_back(params.getInt("x"), params.getInt("y"));
    }
  }
  radius_ = std::max(0, params.getInt("radius", params.getInt("size", radius_)));
  color_ = params.getInt("color", color_);
  alpha_ = params.getInt("alpha", alpha_);
}

bool PrimitiveDots::render(avs::core::RenderContext& context) {
  if (!context.framebuffer.data || context.width <= 0 || context.height <= 0) {
    return true;
  }
  const detail::RGBA color = detail::colorFromInt(color_, detail::clampByte(alpha_));
  if (points_.empty()) {
    return true;
  }
  for (const auto& pt : points_) {
    detail::drawFilledCircle(context, pt.first, pt.second, radius_, color);
  }
  return true;
}

}  // namespace avs::effects

