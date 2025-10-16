#include "effects/stubs/effect_trans_water_bump.h"


Effect_TransWaterBump::Effect_TransWaterBump() : Effect("Trans / Water Bump") {}

bool Effect_TransWaterBump::render(avs::core::RenderContext& context) {
  (void)context;
  // TODO: Implement per R_WaterBump in r_waterbump.cpp
  return true;
}
