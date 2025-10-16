#include "effects/dynamic/dyn_shift.h"

namespace avs::effects {

DynamicShaderEffect::SampleCoord DynamicShiftEffect::resolveSample() const {
  const float baseX = origXVar_ ? static_cast<float>(*origXVar_) : 0.0f;
  const float baseY = origYVar_ ? static_cast<float>(*origYVar_) : 0.0f;
  const float shiftX = dxVar_ ? static_cast<float>(*dxVar_) : 0.0f;
  const float shiftY = dyVar_ ? static_cast<float>(*dyVar_) : 0.0f;
  SampleCoord coord{};
  coord.x = baseX + shiftX;
  coord.y = baseY + shiftY;
  return coord;
}

}  // namespace avs::effects

