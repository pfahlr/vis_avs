#include "avs/effects/RegisterEffects.hpp"

#include <memory>

#include "avs/effects/Blend.hpp"
#include "avs/effects/Clear.hpp"
#include "avs/effects/Overlay.hpp"
#include "avs/effects/Swizzle.hpp"
#include "avs/effects/Zoom.hpp"
#include "effects/effect_scripted.h"

namespace avs::effects {

void registerCoreEffects(avs::core::EffectRegistry& registry) {
  registry.registerFactory("clear", []() { return std::make_unique<Clear>(); });
  registry.registerFactory("zoom", []() { return std::make_unique<Zoom>(); });
  registry.registerFactory("blend", []() { return std::make_unique<Blend>(); });
  registry.registerFactory("overlay", []() { return std::make_unique<Overlay>(); });
  registry.registerFactory("swizzle", []() { return std::make_unique<Swizzle>(); });
  registry.registerFactory("scripted", []() { return std::make_unique<ScriptedEffect>(); });
}

}  // namespace avs::effects
