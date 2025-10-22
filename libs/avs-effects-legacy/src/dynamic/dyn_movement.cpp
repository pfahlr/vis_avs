#include <avs/effects/legacy/dynamic/dyn_movement.h>

namespace avs::effects {

DynamicShaderEffect::SampleCoord DynamicMovementEffect::resolveSample() const {
  SampleCoord coord{};
  coord.x = xVar_ ? static_cast<float>(*xVar_) : 0.0f;
  coord.y = yVar_ ? static_cast<float>(*yVar_) : 0.0f;
  return coord;
}

}  // namespace avs::effects

