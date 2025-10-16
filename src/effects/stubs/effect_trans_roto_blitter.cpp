#include "effects/stubs/effect_trans_roto_blitter.h"


Effect_TransRotoBlitter::Effect_TransRotoBlitter() : Effect("Trans / Roto Blitter") {}

bool Effect_TransRotoBlitter::render(avs::core::RenderContext& context) {
  (void)context;
  // TODO: Implement per R_RotBlit in r_rotblit.cpp
  return true;
}
