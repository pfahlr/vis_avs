#include "effects/stubs/effect_trans_water.h"


Effect_TransWater::Effect_TransWater() : Effect("Trans / Water") {}

bool Effect_TransWater::render(avs::core::RenderContext& context) {
  (void)context;
  // TODO: Implement per R_Water in r_water.cpp
  return true;
}
