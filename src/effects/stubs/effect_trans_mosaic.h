#pragma once

#include "effects/effect.h"

class Effect_TransMosaic : public Effect {
 public:
  Effect_TransMosaic();
  ~Effect_TransMosaic() override = default;

  bool render(avs::core::RenderContext& context) override;
};
