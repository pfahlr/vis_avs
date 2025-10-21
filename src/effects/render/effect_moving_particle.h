#pragma once

#include <array>
#include <cstdint>

#include "effects/effect.h"

class Effect_RenderMovingParticle : public Effect {
 public:
  Effect_RenderMovingParticle();
  ~Effect_RenderMovingParticle() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  enum class BlendMode {
    Replace = 0,
    Additive = 1,
    Average = 2,
    Line = 3,
  };

  void applyBlend(std::uint8_t* pixel) const;
  void applyLineBlend(std::uint8_t* pixel) const;
  void setColorFromInt(int rgb);

  static constexpr int kMaxRadius = 128;
  static constexpr int kMinRadius = 1;
  static constexpr int kMaxDistance = 128;
  static constexpr float kSpringStrength = 0.004f;
  static constexpr float kDamping = 0.991f;

  bool enabled_ = true;
  bool beatPulse_ = false;
  int maxDistance_ = 16;
  int sizeBase_ = 8;
  int sizeBeat_ = 8;
  int sPos_ = 8;
  BlendMode blendMode_ = BlendMode::Additive;
  int lineBlendMode_ = 0;
  int lineBlendAdjust_ = 128;
  std::array<float, 2> target_{0.0f, 0.0f};
  std::array<float, 2> position_{-0.6f, 0.3f};
  std::array<float, 2> velocity_{-0.01551f, 0.0f};
  std::array<std::uint8_t, 3> color_{255u, 255u, 255u};
};
