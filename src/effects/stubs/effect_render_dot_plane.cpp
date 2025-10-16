#include "effects/stubs/effect_render_dot_plane.h"


Effect_RenderDotPlane::Effect_RenderDotPlane() : Effect("Render / Dot Plane") {}

bool Effect_RenderDotPlane::render(avs::core::RenderContext& context) {
  (void)context;
  // TODO: Implement per R_DotPlane in r_dotpln.cpp
  return true;
}
