#pragma once

#include "effects/effect.h"

class Effect_RenderSvpLoader : public Effect {
 public:
  Effect_RenderSvpLoader();
  ~Effect_RenderSvpLoader() override = default;

  bool render(avs::core::RenderContext& context) override;
};
