#pragma once

#include "effects/effect.h"

class Effect_TransWaterBump : public Effect {
 public:
  Effect_TransWaterBump();
  ~Effect_TransWaterBump() override = default;

  bool render(avs::core::RenderContext& context) override;
};
