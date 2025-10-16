#include "effects/stubs/effect_holden05_multi_delay.h"


Effect_Holden05MultiDelay::Effect_Holden05MultiDelay() : Effect("Holden05: Multi Delay") {}

bool Effect_Holden05MultiDelay::render(avs::core::RenderContext& context) {
  (void)context;
  // TODO: Implement per R_MultiDelay in rlib.cpp
  return true;
}
