#pragma once

#include "effects/effect.h"

class Effect_Multiplier : public Effect {
 public:
  Effect_Multiplier();
  ~Effect_Multiplier() override = default;

  bool render(avs::core::RenderContext& context) override;
};
