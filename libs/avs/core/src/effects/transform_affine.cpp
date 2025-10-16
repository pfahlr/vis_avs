#include "transform_affine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <cstdint>

namespace avs::core {
namespace {
struct Color {
  std::uint8_t r;
  std::uint8_t g;
  std::uint8_t b;
};

constexpr Color kTriangleColor{220, 70, 70};
constexpr Color kCrosshairColor{60, 255, 120};
constexpr Color kBackgroundColor{0, 0, 0};
constexpr float kPi = 3.14159265358979323846f;

void fillBackground(PixelBufferView& fb, int width, int height, const Color& color) {
  if (!fb.data || width <= 0 || height <= 0) {
    return;
  }
  std::size_t pixels = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
  for (std::size_t i = 0; i < pixels; ++i) {
    std::uint8_t* px = fb.data + i * 4;
    px[0] = color.r;
    px[1] = color.g;
    px[2] = color.b;
    px[3] = 255u;
  }
}

void putPixel(PixelBufferView& fb, int width, int height, int x, int y, const Color& color) {
  if (x < 0 || y < 0 || x >= width || y >= height) {
    return;
  }
  std::size_t index = static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                      static_cast<std::size_t>(x);
  if (index * 4ull >= fb.size) {
    return;
  }
  std::uint8_t* px = fb.data + index * 4;
  px[0] = color.r;
  px[1] = color.g;
  px[2] = color.b;
  px[3] = 255u;
}

float edgeFunction(const TransformAffineEffect::Vec2& a,
                   const TransformAffineEffect::Vec2& b,
                   const TransformAffineEffect::Vec2& c) {
  return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}

}  // namespace

std::string TransformAffineEffect::toLower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return value;
}

void TransformAffineEffect::setParams(const ParamBlock& params) {
  rotationDeg_ = params.getFloat("rotation_deg", rotationDeg_);
  scaleX_ = params.getFloat("scale_x", scaleX_);
  scaleY_ = params.getFloat("scale_y", scaleY_);
  translateX_ = params.getFloat("translate_x", translateX_);
  translateY_ = params.getFloat("translate_y", translateY_);
  customAnchorX_ = std::clamp(params.getFloat("anchor_x", customAnchorX_), 0.0f, 1.0f);
  customAnchorY_ = std::clamp(params.getFloat("anchor_y", customAnchorY_), 0.0f, 1.0f);
  drawCrosshair_ = params.getBool("crosshair", drawCrosshair_);
  doubleSize_ = params.getBool("double_size", doubleSize_);
  useRandomOffset_ = params.getBool("use_random_offset", useRandomOffset_);
  drawShape_ = params.getBool("draw_shape", drawShape_);
  starCount_ = params.getInt("star_count", starCount_);

  std::string anchor = toLower(params.getString("anchor", "center"));
  if (anchor == "center") {
    anchor_ = AnchorMode::Center;
  } else if (anchor == "top_left" || anchor == "topleft") {
    anchor_ = AnchorMode::TopLeft;
  } else if (anchor == "top_right" || anchor == "topright") {
    anchor_ = AnchorMode::TopRight;
  } else if (anchor == "bottom_left" || anchor == "bottomleft") {
    anchor_ = AnchorMode::BottomLeft;
  } else if (anchor == "bottom_right" || anchor == "bottomright") {
    anchor_ = AnchorMode::BottomRight;
  } else if (anchor == "custom") {
    anchor_ = AnchorMode::Custom;
  }
}

TransformAffineEffect::Vec2 TransformAffineEffect::resolveAnchor(const RenderContext& context) const {
  if (context.width <= 0 || context.height <= 0) {
    return Vec2{};
  }
  float w = static_cast<float>(context.width - 1);
  float h = static_cast<float>(context.height - 1);
  switch (anchor_) {
    case AnchorMode::Center:
      return Vec2{w * 0.5f, h * 0.5f};
    case AnchorMode::TopLeft:
      return Vec2{0.0f, 0.0f};
    case AnchorMode::TopRight:
      return Vec2{w, 0.0f};
    case AnchorMode::BottomLeft:
      return Vec2{0.0f, h};
    case AnchorMode::BottomRight:
      return Vec2{w, h};
    case AnchorMode::Custom:
      return Vec2{w * customAnchorX_, h * customAnchorY_};
  }
  return Vec2{};
}

bool TransformAffineEffect::render(RenderContext& context) {
  auto& fb = context.framebuffer;
  if (!fb.data || fb.size == 0 || context.width <= 0 || context.height <= 0) {
    return true;
  }

  fillBackground(fb, context.width, context.height, kBackgroundColor);

  Vec2 anchor = resolveAnchor(context);

  float offsetX = translateX_;
  float offsetY = translateY_;
  if (useRandomOffset_) {
    offsetX += context.gating.randomOffsetX * static_cast<float>(context.width) * 0.5f;
    offsetY += context.gating.randomOffsetY * static_cast<float>(context.height) * 0.5f;
  }

  float size = static_cast<float>(std::min(context.width, context.height)) * 0.25f;
  float scaleX = scaleX_ * (doubleSize_ ? 2.0f : 1.0f);
  float scaleY = scaleY_ * (doubleSize_ ? 2.0f : 1.0f);

  std::array<Vec2, 3> local = {Vec2{0.0f, -size}, Vec2{size * 0.9f, size * 0.6f}, Vec2{-size * 0.9f, size * 0.6f}};

  float radians = rotationDeg_ * (kPi / 180.0f);
  float cosA = std::cos(radians);
  float sinA = std::sin(radians);

  std::array<Vec2, 3> transformed{};
  for (std::size_t i = 0; i < local.size(); ++i) {
    float lx = local[i].x * scaleX;
    float ly = local[i].y * scaleY;
    float rx = lx * cosA - ly * sinA;
    float ry = lx * sinA + ly * cosA;
    transformed[i].x = anchor.x + offsetX + rx;
    transformed[i].y = anchor.y + offsetY + ry;
  }

  lastTriangle_ = transformed;
  lastAnchor_ = Vec2{anchor.x + offsetX, anchor.y + offsetY};

  if (drawShape_ && context.gating.active) {
    drawTriangle(fb, context.width, context.height, transformed);
  }

  if (starCount_ > 0 && context.gating.active) {
    drawStarfield(context, fb, context.width, context.height);
  }

  if (drawCrosshair_ || context.testMode) {
    drawCrosshair(fb, context.width, context.height, lastAnchor_);
  }

  return true;
}

void TransformAffineEffect::drawTriangle(PixelBufferView& fb, int width, int height,
                                         const std::array<Vec2, 3>& triangle) const {
  float minX = std::min({triangle[0].x, triangle[1].x, triangle[2].x});
  float maxX = std::max({triangle[0].x, triangle[1].x, triangle[2].x});
  float minY = std::min({triangle[0].y, triangle[1].y, triangle[2].y});
  float maxY = std::max({triangle[0].y, triangle[1].y, triangle[2].y});

  int ix0 = std::max(0, static_cast<int>(std::floor(minX)));
  int ix1 = std::min(width - 1, static_cast<int>(std::ceil(maxX)));
  int iy0 = std::max(0, static_cast<int>(std::floor(minY)));
  int iy1 = std::min(height - 1, static_cast<int>(std::ceil(maxY)));

  float area = edgeFunction(triangle[0], triangle[1], triangle[2]);
  if (std::fabs(area) < 1e-5f) {
    return;
  }

  for (int y = iy0; y <= iy1; ++y) {
    for (int x = ix0; x <= ix1; ++x) {
      Vec2 p{static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f};
      float w0 = edgeFunction(triangle[1], triangle[2], p);
      float w1 = edgeFunction(triangle[2], triangle[0], p);
      float w2 = edgeFunction(triangle[0], triangle[1], p);
      bool hasNeg = (w0 < 0) || (w1 < 0) || (w2 < 0);
      bool hasPos = (w0 > 0) || (w1 > 0) || (w2 > 0);
      if (!(hasNeg && hasPos)) {
        putPixel(fb, width, height, x, y, kTriangleColor);
      }
    }
  }
}

void TransformAffineEffect::drawCrosshair(PixelBufferView& fb, int width, int height, const Vec2& anchor) const {
  int ax = std::clamp(static_cast<int>(std::round(anchor.x)), 0, width - 1);
  int ay = std::clamp(static_cast<int>(std::round(anchor.y)), 0, height - 1);
  int radius = std::max(1, std::min(width, height) / 16);
  for (int dx = -radius; dx <= radius; ++dx) {
    putPixel(fb, width, height, ax + dx, ay, kCrosshairColor);
  }
  for (int dy = -radius; dy <= radius; ++dy) {
    putPixel(fb, width, height, ax, ay + dy, kCrosshairColor);
  }
}

void TransformAffineEffect::drawStarfield(RenderContext& context, PixelBufferView& fb, int width, int height) const {
  for (int i = 0; i < starCount_; ++i) {
    float fx = context.rng.uniform(0.0f, static_cast<float>(std::max(width - 1, 0)));
    float fy = context.rng.uniform(0.0f, static_cast<float>(std::max(height - 1, 0)));
    int x = static_cast<int>(std::round(fx));
    int y = static_cast<int>(std::round(fy));
    putPixel(fb, width, height, x, y, Color{255, 255, 255});
  }
}

}  // namespace avs::core
