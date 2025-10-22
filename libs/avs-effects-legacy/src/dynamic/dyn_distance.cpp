#include <avs/effects/legacy/dynamic/dyn_distance.h>

#include <cmath>

namespace avs::effects {

DynamicShaderEffect::SampleCoord DynamicDistanceModifierEffect::resolveSample() const {
  const float radius = radiusVar_ ? static_cast<float>(*radiusVar_) : 0.0f;
  const float angle = angleVar_ ? static_cast<float>(*angleVar_) : 0.0f;
  SampleCoord coord{};
  coord.x = radius * std::cos(angle);
  coord.y = radius * std::sin(angle);
  return coord;
}

}  // namespace avs::effects

