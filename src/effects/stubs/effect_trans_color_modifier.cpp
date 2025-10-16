#include "effects/stubs/effect_trans_color_modifier.h"


Effect_TransColorModifier::Effect_TransColorModifier() : Effect("Trans / Color Modifier") {}

bool Effect_TransColorModifier::render(avs::core::RenderContext& context) {
  (void)context;
  // TODO: Implement per R_DColorMod in r_dcolormod.cpp
  return true;
}
