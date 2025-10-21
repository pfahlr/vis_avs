#pragma once

#include <array>
#include <cstdint>

#include "avs/core/IEffect.hpp"

namespace avs::core {
class ParamBlock;
}

/**
 * @brief Particle fountain effect emulating the legacy AVS "Dot Fountain" renderer.
 */
class Effect_RenderDotFountain : public avs::core::IEffect {
 public:
  Effect_RenderDotFountain();
  ~Effect_RenderDotFountain() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  static constexpr int kDivisionCount = 30;
  static constexpr int kHeightSlices = 256;
  static constexpr float kDegToRad = 3.14159265358979323846f / 180.0f;

  struct FountainPoint {
    float radius = 0.0f;
    float radialVelocity = 0.0f;
    float height = 0.0f;
    float verticalVelocity = 0.0f;
    float axisX = 0.0f;
    float axisY = 0.0f;
    std::array<std::uint8_t, 4> color{0, 0, 0, 255};
  };

  struct Matrix4x4 {
    std::array<float, 16> m{};
  };

  static Matrix4x4 makeIdentityMatrix();
  static Matrix4x4 makeRotationX(float degrees);
  static Matrix4x4 makeRotationY(float degrees);
  static Matrix4x4 makeTranslation(float x, float y, float z);
  static Matrix4x4 multiply(const Matrix4x4& a, const Matrix4x4& b);
  static void transformPoint(const Matrix4x4& m,
                             float x,
                             float y,
                             float z,
                             float& outX,
                             float& outY,
                             float& outZ);

  void rebuildColorTable();
  void spawnNewRing(const avs::core::RenderContext& context,
                    const std::array<FountainPoint, kDivisionCount>& previous);
  [[nodiscard]] int sampleSpectrum(const avs::core::RenderContext& context, int index) const;
  static void blendAdditive(std::uint8_t* pixel, const std::array<std::uint8_t, 4>& color);

  int rotationVelocity_ = 16;
  int tiltAngle_ = -20;
  float rotationDegrees_ = 0.0f;
  std::array<int, 5> palette_ = {0x186B1C, 0x230AFF, 0x741D2A, 0xD93690, 0xFF886B};
  std::array<std::array<std::uint8_t, 4>, 64> colorTable_{};
  std::array<std::array<FountainPoint, kDivisionCount>, kHeightSlices> points_{};
};
