#include "effects/stubs/effect_trans_color_clip.h"


Effect_TransColorClip::Effect_TransColorClip() : Effect("Trans / Color Clip") {}

bool Effect_TransColorClip::render(avs::core::RenderContext& context) {
  (void)context;
  // TODO: Implement per R_ContrastEnhance in r_colorreplace.cpp
  return true;
}
