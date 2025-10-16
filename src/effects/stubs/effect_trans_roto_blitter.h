#pragma once

#include "effects/effect.h"

class Effect_TransRotoBlitter : public Effect {
 public:
  Effect_TransRotoBlitter();
  ~Effect_TransRotoBlitter() override = default;

  bool render(avs::core::RenderContext& context) override;
};
