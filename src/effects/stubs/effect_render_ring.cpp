#include "effects/stubs/effect_render_ring.h"


Effect_RenderRing::Effect_RenderRing() : Effect("Render / Ring") {}

bool Effect_RenderRing::render(avs::core::RenderContext& context) {
  (void)context;
  // TODO: Implement per R_OscRings in r_oscring.cpp
  return true;
}
