#include "effects/render/effect_dot_fountain.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>

#include <avs/core/ParamBlock.hpp>

namespace {
constexpr float kEpsilon = 1e-7f;
constexpr float kTwoPi = 6.28318530717958647692f;
constexpr float kSpectrumScale = 128.0f;

}  // namespace

Effect_RenderDotFountain::Effect_RenderDotFountain() {
  rebuildColorTable();
}

bool Effect_RenderDotFountain::render(avs::core::RenderContext& context) {
  if (!context.framebuffer.data || context.width <= 0 || context.height <= 0) {
    return true;
  }
  const std::size_t required = static_cast<std::size_t>(context.width) *
                               static_cast<std::size_t>(context.height) * 4u;
  if (context.framebuffer.size < required) {
    return true;
  }

  std::array<FountainPoint, kDivisionCount> previous = points_[0];
  for (int layer = kHeightSlices - 2; layer >= 0; --layer) {
    const float booga = 1.3f / (static_cast<float>(layer) + 100.0f);
    for (int division = 0; division < kDivisionCount; ++division) {
      FountainPoint& src = points_[static_cast<std::size_t>(layer)][static_cast<std::size_t>(division)];
      FountainPoint& dst = points_[static_cast<std::size_t>(layer + 1)][static_cast<std::size_t>(division)];
      dst = src;
      dst.radius += dst.radialVelocity;
      dst.verticalVelocity += 0.05f;
      dst.radialVelocity += booga;
      dst.height += dst.verticalVelocity;
    }
  }

  spawnNewRing(context, previous);

  const Matrix4x4 rotY = makeRotationY(rotationDegrees_);
  const Matrix4x4 rotX = makeRotationX(static_cast<float>(tiltAngle_));
  const Matrix4x4 translation = makeTranslation(0.0f, -20.0f, 400.0f);
  const Matrix4x4 transform = multiply(multiply(translation, rotX), rotY);

  float adjWidth = static_cast<float>(context.width) * 440.0f / 640.0f;
  float adjHeight = static_cast<float>(context.height) * 440.0f / 480.0f;
  const float adj = std::min(adjWidth, adjHeight);

  std::uint8_t* framebuffer = context.framebuffer.data;
  const int width = context.width;
  const int height = context.height;
  for (const auto& row : points_) {
    for (const auto& point : row) {
      float x = 0.0f;
      float y = 0.0f;
      float z = 0.0f;
      transformPoint(transform, point.axisX * point.radius, point.height, point.axisY * point.radius, x, y, z);
      if (z <= kEpsilon) {
        continue;
      }
      const float invZ = adj / z;
      if (!(invZ > 0.0f)) {
        continue;
      }
      const int ix = static_cast<int>(x * invZ) + width / 2;
      const int iy = static_cast<int>(y * invZ) + height / 2;
      if (ix < 0 || ix >= width || iy < 0 || iy >= height) {
        continue;
      }
      std::uint8_t* pixel = framebuffer +
                             (static_cast<std::size_t>(iy) * static_cast<std::size_t>(width) +
                              static_cast<std::size_t>(ix)) *
                                 4u;
      blendAdditive(pixel, point.color);
    }
  }

  rotationDegrees_ += static_cast<float>(rotationVelocity_) / 5.0f;
  if (rotationDegrees_ >= 360.0f) {
    rotationDegrees_ = std::fmod(rotationDegrees_, 360.0f);
  }
  if (rotationDegrees_ < 0.0f) {
    rotationDegrees_ += 360.0f;
  }

  return true;
}

void Effect_RenderDotFountain::setParams(const avs::core::ParamBlock& params) {
  if (params.contains("rotvel")) {
    rotationVelocity_ = params.getInt("rotvel", rotationVelocity_);
  } else if (params.contains("rotation_velocity")) {
    rotationVelocity_ = params.getInt("rotation_velocity", rotationVelocity_);
  }
  if (params.contains("angle")) {
    tiltAngle_ = params.getInt("angle", tiltAngle_);
  }
  if (params.contains("rotation")) {
    rotationDegrees_ = params.getFloat("rotation", rotationDegrees_);
  }

  bool paletteChanged = false;
  for (int i = 0; i < 5; ++i) {
    const std::string key = "color" + std::to_string(i);
    if (params.contains(key)) {
      const int value = params.getInt(key, palette_[static_cast<std::size_t>(i)]);
      if (palette_[static_cast<std::size_t>(i)] != value) {
        palette_[static_cast<std::size_t>(i)] = value;
        paletteChanged = true;
      }
    }
  }
  if (paletteChanged) {
    rebuildColorTable();
  }
}

Effect_RenderDotFountain::Matrix4x4 Effect_RenderDotFountain::makeIdentityMatrix() {
  Matrix4x4 matrix{};
  matrix.m[0] = 1.0f;
  matrix.m[5] = 1.0f;
  matrix.m[10] = 1.0f;
  matrix.m[15] = 1.0f;
  return matrix;
}

Effect_RenderDotFountain::Matrix4x4 Effect_RenderDotFountain::makeRotationX(float degrees) {
  Matrix4x4 matrix = makeIdentityMatrix();
  const float radians = degrees * kDegToRad;
  const float c = std::cos(radians);
  const float s = std::sin(radians);
  matrix.m[5] = c;
  matrix.m[6] = s;
  matrix.m[9] = -s;
  matrix.m[10] = c;
  return matrix;
}

Effect_RenderDotFountain::Matrix4x4 Effect_RenderDotFountain::makeRotationY(float degrees) {
  Matrix4x4 matrix = makeIdentityMatrix();
  const float radians = degrees * kDegToRad;
  const float c = std::cos(radians);
  const float s = std::sin(radians);
  matrix.m[0] = c;
  matrix.m[2] = -s;
  matrix.m[8] = s;
  matrix.m[10] = c;
  return matrix;
}

Effect_RenderDotFountain::Matrix4x4 Effect_RenderDotFountain::makeTranslation(float x, float y, float z) {
  Matrix4x4 matrix = makeIdentityMatrix();
  matrix.m[3] = x;
  matrix.m[7] = y;
  matrix.m[11] = z;
  return matrix;
}

Effect_RenderDotFountain::Matrix4x4 Effect_RenderDotFountain::multiply(const Matrix4x4& a,
                                                                       const Matrix4x4& b) {
  Matrix4x4 result{};
  for (int row = 0; row < 4; ++row) {
    for (int col = 0; col < 4; ++col) {
      result.m[row * 4 + col] = a.m[row * 4 + 0] * b.m[0 * 4 + col] +
                                a.m[row * 4 + 1] * b.m[1 * 4 + col] +
                                a.m[row * 4 + 2] * b.m[2 * 4 + col] +
                                a.m[row * 4 + 3] * b.m[3 * 4 + col];
    }
  }
  return result;
}

void Effect_RenderDotFountain::transformPoint(const Matrix4x4& m,
                                              float x,
                                              float y,
                                              float z,
                                              float& outX,
                                              float& outY,
                                              float& outZ) {
  outX = x * m.m[0] + y * m.m[1] + z * m.m[2] + m.m[3];
  outY = x * m.m[4] + y * m.m[5] + z * m.m[6] + m.m[7];
  outZ = x * m.m[8] + y * m.m[9] + z * m.m[10] + m.m[11];
}

void Effect_RenderDotFountain::rebuildColorTable() {
  for (std::size_t segment = 0; segment < 4; ++segment) {
    const int colorA = palette_[segment];
    const int colorB = palette_[segment + 1];
    int r = ((colorA >> 16) & 0xFF) << 16;
    int g = ((colorA >> 8) & 0xFF) << 16;
    int b = (colorA & 0xFF) << 16;
    const int dr = ((((colorB >> 16) & 0xFF) - ((colorA >> 16) & 0xFF)) << 16) / 16;
    const int dg = ((((colorB >> 8) & 0xFF) - ((colorA >> 8) & 0xFF)) << 16) / 16;
    const int db = (((colorB & 0xFF) - (colorA & 0xFF)) << 16) / 16;
    for (int step = 0; step < 16; ++step) {
      const std::size_t index = segment * 16u + static_cast<std::size_t>(step);
      const int rr = std::clamp(r >> 16, 0, 255);
      const int gg = std::clamp(g >> 16, 0, 255);
      const int bb = std::clamp(b >> 16, 0, 255);
      colorTable_[index] = {static_cast<std::uint8_t>(rr), static_cast<std::uint8_t>(gg),
                            static_cast<std::uint8_t>(bb), 255u};
      r += dr;
      g += dg;
      b += db;
    }
  }
}

void Effect_RenderDotFountain::spawnNewRing(
    const avs::core::RenderContext& context,
    const std::array<FountainPoint, kDivisionCount>& previous) {
  auto& row = points_[0];
  const bool beat = context.audioBeat;
  const float angleStep = kTwoPi / static_cast<float>(kDivisionCount);

  for (int division = 0; division < kDivisionCount; ++division) {
    FountainPoint& point = row[static_cast<std::size_t>(division)];
    const FountainPoint& prior = previous[static_cast<std::size_t>(division)];
    const float previousVelocity = point.verticalVelocity;

    int t = sampleSpectrum(context, division) ^ 0x80;
    t = (t * 5) / 4;
    t -= 64;
    if (beat) {
      t += 128;
    }
    t = std::clamp(t, 0, 255);

    point.radius = 1.0f;
    float dr = static_cast<float>(t) / 200.0f;
    if (dr < 0.0f) {
      dr = -dr;
    }
    point.height = 250.0f;
    dr += 1.0f;
    point.verticalVelocity =
        -dr * (100.0f + (previousVelocity - prior.verticalVelocity)) / 100.0f * 2.8f;

    const int colorIndex = std::clamp(t / 4, 0, 63);
    point.color = colorTable_[static_cast<std::size_t>(colorIndex)];
    const float angle = angleStep * static_cast<float>(division);
    point.axisX = std::sin(angle);
    point.axisY = std::cos(angle);
    point.radialVelocity = 0.0f;
  }
}

int Effect_RenderDotFountain::sampleSpectrum(const avs::core::RenderContext& context,
                                             int index) const {
  const auto& spectrum = context.audioSpectrum;
  if (!spectrum.data || spectrum.size == 0) {
    return 0;
  }
  const std::size_t sampleIndex = std::min<std::size_t>(
      spectrum.size - 1,
      static_cast<std::size_t>(static_cast<std::size_t>(index) * spectrum.size /
                               kDivisionCount));
  float magnitude = spectrum.data[sampleIndex];
  if (!std::isfinite(magnitude)) {
    magnitude = 0.0f;
  }
  if (magnitude < 0.0f) {
    magnitude = 0.0f;
  }
  const float scaled = std::clamp(magnitude * kSpectrumScale, 0.0f, 127.0f);
  return static_cast<int>(std::lround(scaled));
}

void Effect_RenderDotFountain::blendAdditive(std::uint8_t* pixel,
                                             const std::array<std::uint8_t, 4>& color) {
  for (int channel = 0; channel < 3; ++channel) {
    const int value = static_cast<int>(pixel[channel]) + static_cast<int>(color[channel]);
    pixel[channel] = static_cast<std::uint8_t>(value > 255 ? 255 : value);
  }
  const int alpha = static_cast<int>(pixel[3]) + static_cast<int>(color[3]);
  pixel[3] = static_cast<std::uint8_t>(alpha > 255 ? 255 : alpha);
}
