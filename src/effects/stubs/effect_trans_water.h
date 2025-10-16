#pragma once

#include "effects/effect.h"

class Effect_TransWater : public Effect {
 public:
  Effect_TransWater();
  ~Effect_TransWater() override = default;

  bool render(avs::core::RenderContext& context) override;
};
