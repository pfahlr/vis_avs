#include "effects/stubs/effect_trans_unique_tone.h"


Effect_TransUniqueTone::Effect_TransUniqueTone() : Effect("Trans / Unique tone") {}

bool Effect_TransUniqueTone::render(avs::core::RenderContext& context) {
  (void)context;
  // TODO: Implement per R_Onetone in r_onetone.cpp
  return true;
}
