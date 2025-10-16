#include "effects/stubs/effect_render_avi.h"


Effect_RenderAvi::Effect_RenderAvi() : Effect("Render / AVI") {}

bool Effect_RenderAvi::render(avs::core::RenderContext& context) {
  (void)context;
  // TODO: Implement per R_AVI in r_avi.cpp
  return true;
}
