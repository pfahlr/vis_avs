#include "effects/stubs/effect_render_timescope.h"


Effect_RenderTimescope::Effect_RenderTimescope() : Effect("Render / Timescope") {}

bool Effect_RenderTimescope::render(avs::core::RenderContext& context) {
  (void)context;
  // TODO: Implement per R_Timescope in r_timescope.cpp
  return true;
}
