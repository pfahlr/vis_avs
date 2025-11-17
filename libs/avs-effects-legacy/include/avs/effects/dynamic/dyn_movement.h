#pragma once

#include <avs/effects/dynamic/dynamic_shader.h>

namespace avs::effects {

class DynamicMovementEffect : public DynamicShaderEffect {
 public:
  DynamicMovementEffect() = default;
  ~DynamicMovementEffect() override = default;

 protected:
  SampleCoord resolveSample() const override;
};

}  // namespace avs::effects

