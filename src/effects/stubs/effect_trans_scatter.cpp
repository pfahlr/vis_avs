#include "effects/stubs/effect_trans_scatter.h"


Effect_TransScatter::Effect_TransScatter() : Effect("Trans / Scatter") {}

bool Effect_TransScatter::render(avs::core::RenderContext& context) {
  (void)context;
  // TODO: Implement per R_Scat in r_scat.cpp
  return true;
}
