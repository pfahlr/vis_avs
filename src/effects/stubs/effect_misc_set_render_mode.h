#pragma once

#include "effects/effect.h"

class Effect_MiscSetRenderMode : public Effect {
 public:
  Effect_MiscSetRenderMode();
  ~Effect_MiscSetRenderMode() override = default;

  bool render(avs::core::RenderContext& context) override;
};
