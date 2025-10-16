#include "effects/stubs/effect_render_simple.h"


Effect_RenderSimple::Effect_RenderSimple() : Effect("Render / Simple") {}

bool Effect_RenderSimple::render(avs::core::RenderContext& context) {
  (void)context;
  // TODO: Implement per R_SimpleSpectrum in r_simple.cpp
  return true;
}
