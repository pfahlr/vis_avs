#pragma once

#include <avs/effects/legacy/dynamic/dynamic_shader.h>

namespace avs::effects {

class DynamicShiftEffect : public DynamicShaderEffect {
 public:
  DynamicShiftEffect() = default;
  ~DynamicShiftEffect() override = default;

 protected:
  SampleCoord resolveSample() const override;
};

}  // namespace avs::effects

