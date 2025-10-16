#include "effects/stubs/effect_trans_colorfade.h"


Effect_TransColorfade::Effect_TransColorfade() : Effect("Trans / Colorfade") {}

bool Effect_TransColorfade::render(avs::core::RenderContext& context) {
  (void)context;
  // TODO: Implement per R_ColorFade in r_colorfade.cpp
  return true;
}
