#pragma once

#include "effects/effect.h"

class Effect_Holden04VideoDelay : public Effect {
 public:
  Effect_Holden04VideoDelay();
  ~Effect_Holden04VideoDelay() override = default;

  bool render(avs::core::RenderContext& context) override;
};
