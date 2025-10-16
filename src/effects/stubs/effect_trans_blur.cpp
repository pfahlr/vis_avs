#include "effects/stubs/effect_trans_blur.h"


Effect_TransBlur::Effect_TransBlur() : Effect("Trans / Blur") {}

bool Effect_TransBlur::render(avs::core::RenderContext& context) {
  (void)context;
  // TODO: Implement per R_Blur in r_blur.cpp
  return true;
}
