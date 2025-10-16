#pragma once

#include "effects/effect.h"

class Effect_TransBlur : public Effect {
 public:
  Effect_TransBlur();
  ~Effect_TransBlur() override = default;

  bool render(avs::core::RenderContext& context) override;
};
