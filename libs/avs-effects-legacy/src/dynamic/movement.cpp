#include <avs/effects/legacy/dynamic/movement.h>

#include <algorithm>
#include <cmath>

namespace avs::effects {

namespace {
constexpr double kDegToRad = 3.14159265358979323846 / 180.0;
}

void MovementEffect::setParams(const avs::core::ParamBlock& params) {
  if (params.contains("scale")) {
    scale_ = std::max(0.0001f, params.getFloat("scale", scale_));
  }
  if (params.contains("rotate")) {
    rotationDeg_ = params.getFloat("rotate", rotationDeg_);
  }
  if (params.contains("offset_x")) {
    offsetX_ = params.getFloat("offset_x", offsetX_);
  }
  if (params.contains("offset_y")) {
    offsetY_ = params.getFloat("offset_y", offsetY_);
  }
  if (params.contains("wrap")) {
    wrap_ = params.getBool("wrap", wrap_);
  }
}

bool MovementEffect::render(avs::core::RenderContext& context) {
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
  const float invScale = 1.0f / std::max(scale_, 0.0001f);

  for (int py = 0; py < height; ++py) {
    for (int px = 0; px < width; ++px) {
      const float normX = (static_cast<float>(px) + 0.5f) / static_cast<float>(width);
      const float normY = (static_cast<float>(py) + 0.5f) / static_cast<float>(height);
      float x = normX * 2.0f - 1.0f;
      float y = 1.0f - normY * 2.0f;

      x -= offsetX_;
      y -= offsetY_;
      x *= invScale;
      y *= invScale;

      const float rx = x * cosR - y * sinR;
      const float ry = x * sinR + y * cosR;

      const auto color = sampleHistory(rx, ry, wrap_);
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

