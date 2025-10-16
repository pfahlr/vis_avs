#include "effects/stubs/effect_misc_set_render_mode.h"


Effect_MiscSetRenderMode::Effect_MiscSetRenderMode() : Effect("Misc / Set render mode") {}

bool Effect_MiscSetRenderMode::render(avs::core::RenderContext& context) {
  (void)context;
  // TODO: Implement per R_LineMode in r_linemode.cpp
  return true;
}
