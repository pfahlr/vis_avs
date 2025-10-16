#include "effects/stubs/effect_holden04_video_delay.h"


Effect_Holden04VideoDelay::Effect_Holden04VideoDelay() : Effect("Holden04: Video Delay") {}

bool Effect_Holden04VideoDelay::render(avs::core::RenderContext& context) {
  (void)context;
  // TODO: Implement per R_VideoDelay in rlib.cpp
  return true;
}
