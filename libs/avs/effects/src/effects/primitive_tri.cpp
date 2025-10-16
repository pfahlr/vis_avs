#include "avs/effects/Primitives.hpp"

#include <algorithm>

#include "primitive_common.hpp"

namespace avs::effects {

void PrimitiveTriangles::setParams(const avs::core::ParamBlock& params) {
  triangles_.clear();
  const std::string list = params.getString("triangles", params.getString("points", ""));
  if (!list.empty()) {
    auto parsed = detail::parsePointList(list);
    for (std::size_t i = 0; i + 2 < parsed.size(); i += 3) {
      const auto& p0 = parsed[i];
      const auto& p1 = parsed[i + 1];
      const auto& p2 = parsed[i + 2];
      triangles_.push_back({p0.x, p0.y, p1.x, p1.y, p2.x, p2.y});
    }
  }
  if (triangles_.empty()) {
    if (params.contains("x1") && params.contains("y1") && params.contains("x2") && params.contains("y2") &&
        params.contains("x3") && params.contains("y3")) {
      triangles_.push_back({params.getInt("x1"), params.getInt("y1"), params.getInt("x2"), params.getInt("y2"),
                            params.getInt("x3"), params.getInt("y3")});
    }
  }
  filled_ = params.getBool("filled", filled_);
  color_ = params.getInt("color", color_);
  alpha_ = params.getInt("alpha", alpha_);
  outlineColor_ = params.getInt("outlinecolor", outlineColor_);
  outlineAlpha_ = params.getInt("outlinealpha", outlineAlpha_);
  outlineWidth_ = std::max(0, params.getInt("outlinesize", params.getInt("outlinewidth", outlineWidth_)));
}

bool PrimitiveTriangles::render(avs::core::RenderContext& context) {
  if (!context.framebuffer.data || context.width <= 0 || context.height <= 0) {
    return true;
  }
  const detail::RGBA fillColor = detail::colorFromInt(color_, detail::clampByte(alpha_));
  const detail::RGBA outlineColor = detail::colorFromInt(outlineColor_, detail::clampByte(outlineAlpha_));
  for (const auto& tri : triangles_) {
    const detail::Point p0{tri[0], tri[1]};
    const detail::Point p1{tri[2], tri[3]};
    const detail::Point p2{tri[4], tri[5]};
    if (filled_) {
      int minX = std::min({p0.x, p1.x, p2.x});
      int maxX = std::max({p0.x, p1.x, p2.x});
      int minY = std::min({p0.y, p1.y, p2.y});
      int maxY = std::max({p0.y, p1.y, p2.y});
      minX = std::max(0, minX);
      minY = std::max(0, minY);
      maxX = std::min(context.width - 1, maxX);
      maxY = std::min(context.height - 1, maxY);
      for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
          if (detail::pointInTriangle(p0, p1, p2, x, y)) {
            detail::blendPixel(context, x, y, fillColor);
          }
        }
      }
    }
    if (!filled_ || outlineWidth_ > 0) {
      const int width = std::max(1, outlineWidth_);
      detail::drawThickLine(context, p0.x, p0.y, p1.x, p1.y, width, outlineColor);
      detail::drawThickLine(context, p1.x, p1.y, p2.x, p2.y, width, outlineColor);
      detail::drawThickLine(context, p2.x, p2.y, p0.x, p0.y, width, outlineColor);
    }
  }
  return true;
}

}  // namespace avs::effects

