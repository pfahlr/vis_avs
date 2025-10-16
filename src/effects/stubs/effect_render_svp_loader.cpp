#include "effects/stubs/effect_render_svp_loader.h"


Effect_RenderSvpLoader::Effect_RenderSvpLoader() : Effect("Render / SVP Loader") {}

bool Effect_RenderSvpLoader::render(avs::core::RenderContext& context) {
  (void)context;
  // TODO: Implement per R_SVP in r_svp.cpp
  return true;
}
