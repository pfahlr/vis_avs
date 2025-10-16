#include "avs/effects/Primitives.hpp"

#include <algorithm>

#include "primitive_common.hpp"

namespace avs::effects {

void PrimitiveLines::setParams(const avs::core::ParamBlock& params) {
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
    if (params.contains("x1") && params.contains("y1") && params.contains("x2") && params.contains("y2")) {
      points_.emplace_back(params.getInt("x1"), params.getInt("y1"));
      points_.emplace_back(params.getInt("x2"), params.getInt("y2"));
    }
  }
  closed_ = params.getBool("closed", closed_);
  width_ = std::max(1, params.getInt("width", params.getInt("thickness", width_)));
  color_ = params.getInt("color", color_);
  alpha_ = params.getInt("alpha", alpha_);
}

bool PrimitiveLines::render(avs::core::RenderContext& context) {
  if (!context.framebuffer.data || context.width <= 0 || context.height <= 0) {
    return true;
  }
  if (points_.size() < 2) {
    return true;
  }
  const detail::RGBA color = detail::colorFromInt(color_, detail::clampByte(alpha_));
  for (std::size_t i = 1; i < points_.size(); ++i) {
    const auto& a = points_[i - 1];
    const auto& b = points_[i];
    detail::drawThickLine(context, a.first, a.second, b.first, b.second, width_, color);
  }
  if (closed_ && points_.size() > 2) {
    const auto& first = points_.front();
    const auto& last = points_.back();
    detail::drawThickLine(context, last.first, last.second, first.first, first.second, width_, color);
  }
  return true;
}

}  // namespace avs::effects

