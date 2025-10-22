#pragma once

#include <cstddef>

#include <avs/core/IEffect.hpp>
#include <avs/effects/Gating.hpp>

namespace avs::core {
class ParamBlock;
}

namespace avs::effects::misc {

class CustomBpmEffect : public avs::core::IEffect {
 public:
  CustomBpmEffect();

  void setParams(const avs::core::ParamBlock& params) override;
  bool render(avs::core::RenderContext& context) override;

 private:
  enum class Mode {
    Passthrough,
    Arbitrary,
    Skip,
    Invert,
  };

  void configureGate();
  void resetState();
  [[nodiscard]] double intervalSeconds() const;
  void writeGateRegisters(avs::core::RenderContext& context, const avs::effects::GateResult& gate) const;

  bool enabled_;
  Mode mode_;
  float bpm_;
  int skipInterval_;
  int skipFirstCount_;
  int beatsSeen_;
  int skipCounter_;
  double accumulatorSeconds_;

  avs::effects::GateOptions gateOptions_;
  avs::effects::BeatGate gate_;
  int gateRenderRegister_;
  int gateFlagRegister_;
};

}  // namespace avs::effects::misc
