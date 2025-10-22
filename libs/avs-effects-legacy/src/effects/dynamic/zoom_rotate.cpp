#include "effects/dynamic/zoom_rotate.h"

#include <algorithm>
#include <cmath>

namespace avs::effects {

namespace {
constexpr double kDegToRad = 3.14159265358979323846 / 180.0;
}

void ZoomRotateEffect::setParams(const avs::core::ParamBlock& params) {
  if (params.contains("zoom")) {
    zoom_ = std::max(0.0001f, params.getFloat("zoom", zoom_));
  }
  if (params.contains("rotate")) {
    rotationDeg_ = params.getFloat("rotate", rotationDeg_);
  }
  if (params.contains("anchor_x")) {
    anchorX_ = std::clamp(params.getFloat("anchor_x", anchorX_), 0.0f, 1.0f);
  }
  if (params.contains("anchor_y")) {
    anchorY_ = std::clamp(params.getFloat("anchor_y", anchorY_), 0.0f, 1.0f);
  }
  if (params.contains("wrap")) {
    wrap_ = params.getBool("wrap", wrap_);
  }
}

bool ZoomRotateEffect::render(avs::core::RenderContext& context) {
  if (!prepareHistory(context)) {
    return true;
  }
  const int width = historyWidth();
  const int height = historyHeight();
  if (width <= 0 || height <= 0) {
    return true;
  }

  const double radians = rotationDeg_ * kDegToRad;
  const float cosR = static_cast<float>(std::cos(radians));
  const float sinR = static_cast<float>(std::sin(radians));
  const float invZoom = 1.0f / std::max(zoom_, 0.0001f);

  const float anchorNormX = anchorX_ * 2.0f - 1.0f;
  const float anchorNormY = 1.0f - anchorY_ * 2.0f;

  for (int py = 0; py < height; ++py) {
    for (int px = 0; px < width; ++px) {
      const float normX = (static_cast<float>(px) + 0.5f) / static_cast<float>(width);
      const float normY = (static_cast<float>(py) + 0.5f) / static_cast<float>(height);
      float x = normX * 2.0f - 1.0f;
      float y = 1.0f - normY * 2.0f;

      x -= anchorNormX;
      y -= anchorNormY;
      x *= invZoom;
      y *= invZoom;

      const float rx = x * cosR - y * sinR;
      const float ry = x * sinR + y * cosR;

      const float sampleX = rx + anchorNormX;
      const float sampleY = ry + anchorNormY;

      const auto color = sampleHistory(sampleX, sampleY, wrap_);
      const std::size_t index = (static_cast<std::size_t>(py) * static_cast<std::size_t>(width) +
                                 static_cast<std::size_t>(px)) * 4u;
      context.framebuffer.data[index + 0] = color[0];
      context.framebuffer.data[index + 1] = color[1];
      context.framebuffer.data[index + 2] = color[2];
      context.framebuffer.data[index + 3] = color[3];
    }
  }

  storeHistory(context);
  return true;
}

}  // namespace avs::effects

