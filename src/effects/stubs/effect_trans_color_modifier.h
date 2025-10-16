#pragma once

#include "effects/effect.h"

class Effect_TransColorModifier : public Effect {
 public:
  Effect_TransColorModifier();
  ~Effect_TransColorModifier() override = default;

  bool render(avs::core::RenderContext& context) override;
};
