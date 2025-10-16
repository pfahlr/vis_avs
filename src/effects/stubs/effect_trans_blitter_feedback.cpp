#include "effects/stubs/effect_trans_blitter_feedback.h"


Effect_TransBlitterFeedback::Effect_TransBlitterFeedback() : Effect("Trans / Blitter Feedback") {}

bool Effect_TransBlitterFeedback::render(avs::core::RenderContext& context) {
  (void)context;
  // TODO: Implement per R_BlitterFB in r_blit.cpp
  return true;
}
