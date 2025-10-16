#include "effects/stubs/effect_render_moving_particle.h"


Effect_RenderMovingParticle::Effect_RenderMovingParticle() : Effect("Render / Moving Particle") {}

bool Effect_RenderMovingParticle::render(avs::core::RenderContext& context) {
  (void)context;
  // TODO: Implement per R_Parts in r_parts.cpp
  return true;
}
