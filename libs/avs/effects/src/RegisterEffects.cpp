#include "avs/effects/RegisterEffects.hpp"

#include <memory>

#include "avs/effects/Clear.hpp"
#include "avs/effects/Zoom.hpp"

namespace avs::effects {

void registerCoreEffects(avs::core::EffectRegistry& registry) {
  registry.registerFactory("clear", []() { return std::make_unique<Clear>(); });
  registry.registerFactory("zoom", []() { return std::make_unique<Zoom>(); });
}

}  // namespace avs::effects
