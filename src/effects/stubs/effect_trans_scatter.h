#pragma once

#include "effects/effect.h"

class Effect_TransScatter : public Effect {
 public:
  Effect_TransScatter();
  ~Effect_TransScatter() override = default;

  bool render(avs::core::RenderContext& context) override;
};
