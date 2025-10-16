#include "effects/stubs/effect_trans_brightness.h"


Effect_TransBrightness::Effect_TransBrightness() : Effect("Trans / Brightness") {}

bool Effect_TransBrightness::render(avs::core::RenderContext& context) {
  (void)context;
  // TODO: Implement per R_Brightness in r_bright.cpp
  return true;
}
