#include "effects/stubs/effect_render_bass_spin.h"


Effect_RenderBassSpin::Effect_RenderBassSpin() : Effect("Render / Bass Spin") {}

bool Effect_RenderBassSpin::render(avs::core::RenderContext& context) {
  (void)context;
  // TODO: Implement per R_BSpin in r_bspin.cpp
  return true;
}
