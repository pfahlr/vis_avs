#pragma once

#include "effects/effect.h"

class Effect_Holden05MultiDelay : public Effect {
 public:
  Effect_Holden05MultiDelay();
  ~Effect_Holden05MultiDelay() override = default;

  bool render(avs::core::RenderContext& context) override;
};
