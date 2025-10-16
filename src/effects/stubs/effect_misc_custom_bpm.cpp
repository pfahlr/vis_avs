#include "effects/stubs/effect_misc_custom_bpm.h"


Effect_MiscCustomBpm::Effect_MiscCustomBpm() : Effect("Misc / Custom BPM") {}

bool Effect_MiscCustomBpm::render(avs::core::RenderContext& context) {
  (void)context;
  // TODO: Implement per R_Bpm in r_bpm.cpp
  return true;
}
