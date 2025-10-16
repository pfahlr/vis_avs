#pragma once

#include <array>
#include <deque>
#include <string>

#include "avs/core/IEffect.hpp"
#include "avs/effects/Gating.hpp"

namespace avs::effects {

class TransformAffine : public avs::core::IEffect {
 public:
  TransformAffine();

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  struct RandomConfig {
    float angleAmplitude{0.0f};
    float scaleAmplitude{0.0f};
    float offsetAmplitude{0.0f};
  };

  void updateRandom(avs::core::RenderContext& context, bool beatTriggered);
  void drawTriangle(avs::core::RenderContext& context, const std::array<std::array<float, 2>, 3>& vertices,
                    bool fiftyBlend) const;
  void drawCrosshair(avs::core::RenderContext& context, float x, float y) const;
  void drawGatingLog(avs::core::RenderContext& context) const;

  std::array<float, 2> anchorNorm_{0.0f, 0.0f};
  float baseAngleDeg_{0.0f};
  float rotateSpeedDeg_{0.0f};
  float scale_{1.0f};
  bool doubleSize_{false};
  bool testMode_{false};
  bool randomPosition_{false};
  bool fiftyBlend_{false};
  int logRows_{1};

  std::array<float, 2> jitter_{0.0f, 0.0f};
  float randomAngleOffset_{0.0f};
  float randomScaleFactor_{1.0f};

  RandomConfig random_{};

  struct GateState {
    GateFlag flag{GateFlag::Off};
  };
  std::deque<GateState> history_;
  std::size_t historyLimit_{0};
  std::array<std::uint8_t, 4> color_{255, 0, 0, 255};
  std::array<std::uint8_t, 4> crossColor_{255, 255, 255, 255};

  struct GateStateConfig {
    GateOptions options{};
    BeatGate gate{};
  } gateConfig_{};
};

}  // namespace avs::effects
