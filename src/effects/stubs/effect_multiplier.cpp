#include "effects/stubs/effect_multiplier.h"


Effect_Multiplier::Effect_Multiplier() : Effect("Multiplier") {}

bool Effect_Multiplier::render(avs::core::RenderContext& context) {
  (void)context;
  // TODO: Implement per R_Multiplier in rlib.cpp
  return true;
}
