#pragma once

#include <array>

namespace avs::effects {

struct Affine2D {
  float m00{1.0f}, m01{0.0f}, m02{0.0f};
  float m10{0.0f}, m11{1.0f}, m12{0.0f};

  static Affine2D identity();
  static Affine2D translation(float tx, float ty);
  static Affine2D scale(float sx, float sy);
  static Affine2D rotation(float radians);

  [[nodiscard]] Affine2D operator*(const Affine2D& other) const;
  [[nodiscard]] std::array<float, 2> apply(const std::array<float, 2>& point) const;
};

struct Triangle2D {
  std::array<float, 2> a{};
  std::array<float, 2> b{};
  std::array<float, 2> c{};
};

[[nodiscard]] float signedArea(const Triangle2D& tri);
[[nodiscard]] bool containsPoint(const Triangle2D& tri, const std::array<float, 2>& point);

}  // namespace avs::effects
