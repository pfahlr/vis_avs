#pragma once

#include <avs/effects/legacy/dynamic/dynamic_shader.h>

namespace avs::effects {

class DynamicDistanceModifierEffect : public DynamicShaderEffect {
 public:
  DynamicDistanceModifierEffect() = default;
  ~DynamicDistanceModifierEffect() override = default;

 protected:
  SampleCoord resolveSample() const override;
};

}  // namespace avs::effects

