#include "effects/stubs/effect_render_oscilloscope_star.h"


Effect_RenderOscilloscopeStar::Effect_RenderOscilloscopeStar() : Effect("Render / Oscilloscope Star") {}

bool Effect_RenderOscilloscopeStar::render(avs::core::RenderContext& context) {
  (void)context;
  // TODO: Implement per R_OscStars in r_oscstar.cpp
  return true;
}
