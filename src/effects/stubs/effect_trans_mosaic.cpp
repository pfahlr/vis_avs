#include "effects/stubs/effect_trans_mosaic.h"


Effect_TransMosaic::Effect_TransMosaic() : Effect("Trans / Mosaic") {}

bool Effect_TransMosaic::render(avs::core::RenderContext& context) {
  (void)context;
  // TODO: Implement per R_Mosaic in r_mosaic.cpp
  return true;
}
