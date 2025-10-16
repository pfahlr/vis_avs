#include "effects/stubs/effect_color_reduction.h"


Effect_ColorReduction::Effect_ColorReduction() : Effect("Color Reduction") {}

bool Effect_ColorReduction::render(avs::core::RenderContext& context) {
  (void)context;
  // TODO: Implement per R_ColorReduction in rlib.cpp
  return true;
}
