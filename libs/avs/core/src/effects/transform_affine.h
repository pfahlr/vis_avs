#pragma once

#include <array>
#include <string>

#include "avs/core/IEffect.hpp"

namespace avs::core {

class TransformAffineEffect : public IEffect {
 public:
  struct Vec2 {
    float x{0.0f};
    float y{0.0f};
  };

  TransformAffineEffect() = default;

  bool render(RenderContext& context) override;
  void setParams(const ParamBlock& params) override;

  [[nodiscard]] const std::array<Vec2, 3>& lastTriangle() const { return lastTriangle_; }
  [[nodiscard]] Vec2 lastAnchor() const { return lastAnchor_; }

 private:
  enum class AnchorMode { Center, TopLeft, TopRight, BottomLeft, BottomRight, Custom };

  Vec2 resolveAnchor(const RenderContext& context) const;
  void drawTriangle(PixelBufferView& fb, int width, int height,
                    const std::array<Vec2, 3>& triangle) const;
  void drawCrosshair(PixelBufferView& fb, int width, int height, const Vec2& anchor) const;
  void drawStarfield(RenderContext& context, PixelBufferView& fb, int width, int height) const;
  static std::string toLower(std::string value);

  float rotationDeg_{0.0f};
  float scaleX_{1.0f};
  float scaleY_{1.0f};
  float translateX_{0.0f};
  float translateY_{0.0f};
  float customAnchorX_{0.5f};
  float customAnchorY_{0.5f};
  bool drawCrosshair_{false};
  bool doubleSize_{false};
  bool useRandomOffset_{false};
  bool drawShape_{true};
  int starCount_{0};
  AnchorMode anchor_{AnchorMode::Center};
  mutable std::array<Vec2, 3> lastTriangle_{};
  mutable Vec2 lastAnchor_{};
};

}  // namespace avs::core
