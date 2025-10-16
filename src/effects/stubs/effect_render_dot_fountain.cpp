#include "effects/stubs/effect_render_dot_fountain.h"


Effect_RenderDotFountain::Effect_RenderDotFountain() : Effect("Render / Dot Fountain") {}

bool Effect_RenderDotFountain::render(avs::core::RenderContext& context) {
  (void)context;
  // TODO: Implement per R_DotFountain in r_dotfnt.cpp
  return true;
}
