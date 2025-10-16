#include "effects/stubs/effect_render_rotating_stars.h"


Effect_RenderRotatingStars::Effect_RenderRotatingStars() : Effect("Render / Rotating Stars") {}

bool Effect_RenderRotatingStars::render(avs::core::RenderContext& context) {
  (void)context;
  // TODO: Implement per R_RotStar in r_rotstar.cpp
  return true;
}
