#pragma once

#include "effects/effect.h"

class Effect_ColorReduction : public Effect {
 public:
  Effect_ColorReduction();
  ~Effect_ColorReduction() override = default;

  bool render(avs::core::RenderContext& context) override;
};
