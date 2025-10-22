#include <avs/effects/legacy/transform_affine.h>

#include <cmath>

namespace avs::effects {

Affine2D Affine2D::identity() { return {}; }

Affine2D Affine2D::translation(float tx, float ty) {
  Affine2D t;
  t.m02 = tx;
  t.m12 = ty;
  return t;
}

Affine2D Affine2D::scale(float sx, float sy) {
  Affine2D s;
  s.m00 = sx;
  s.m11 = sy;
  return s;
}

Affine2D Affine2D::rotation(float radians) {
  Affine2D r;
  const float c = std::cos(radians);
  const float s = std::sin(radians);
  r.m00 = c;
  r.m01 = -s;
  r.m10 = s;
  r.m11 = c;
  return r;
}

Affine2D Affine2D::operator*(const Affine2D& other) const {
  Affine2D result;
  result.m00 = m00 * other.m00 + m01 * other.m10;
  result.m01 = m00 * other.m01 + m01 * other.m11;
  result.m02 = m00 * other.m02 + m01 * other.m12 + m02;

  result.m10 = m10 * other.m00 + m11 * other.m10;
  result.m11 = m10 * other.m01 + m11 * other.m11;
  result.m12 = m10 * other.m02 + m11 * other.m12 + m12;
  return result;
}

std::array<float, 2> Affine2D::apply(const std::array<float, 2>& point) const {
  return {m00 * point[0] + m01 * point[1] + m02, m10 * point[0] + m11 * point[1] + m12};
}

static float cross(const std::array<float, 2>& a, const std::array<float, 2>& b) {
  return a[0] * b[1] - a[1] * b[0];
}

float signedArea(const Triangle2D& tri) {
  const std::array<float, 2> ab{tri.b[0] - tri.a[0], tri.b[1] - tri.a[1]};
  const std::array<float, 2> ac{tri.c[0] - tri.a[0], tri.c[1] - tri.a[1]};
  return cross(ab, ac) * 0.5f;
}

bool containsPoint(const Triangle2D& tri, const std::array<float, 2>& point) {
  const std::array<float, 2> v0{tri.c[0] - tri.a[0], tri.c[1] - tri.a[1]};
  const std::array<float, 2> v1{tri.b[0] - tri.a[0], tri.b[1] - tri.a[1]};
  const std::array<float, 2> v2{point[0] - tri.a[0], point[1] - tri.a[1]};

  const float dot00 = v0[0] * v0[0] + v0[1] * v0[1];
  const float dot01 = v0[0] * v1[0] + v0[1] * v1[1];
  const float dot02 = v0[0] * v2[0] + v0[1] * v2[1];
  const float dot11 = v1[0] * v1[0] + v1[1] * v1[1];
  const float dot12 = v1[0] * v2[0] + v1[1] * v2[1];

  const float denom = dot00 * dot11 - dot01 * dot01;
  if (std::fabs(denom) < 1e-6f) {
    return false;
  }
  const float invDenom = 1.0f / denom;
  const float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
  const float v = (dot00 * dot12 - dot01 * dot02) * invDenom;

  return (u >= -1e-4f) && (v >= -1e-4f) && (u + v <= 1.0001f);
}

}  // namespace avs::effects
