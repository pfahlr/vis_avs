#include "effects/stubs/effect_misc_comment.h"


Effect_MiscComment::Effect_MiscComment() : Effect("Misc / Comment") {}

bool Effect_MiscComment::render(avs::core::RenderContext& context) {
  (void)context;
  // TODO: Implement per R_Comment in r_comment.cpp
  return true;
}
