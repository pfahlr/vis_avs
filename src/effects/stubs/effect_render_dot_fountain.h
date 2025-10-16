#pragma once

#include "effects/effect.h"

class Effect_RenderDotFountain : public Effect {
 public:
  Effect_RenderDotFountain();
  ~Effect_RenderDotFountain() override = default;

  bool render(avs::core::RenderContext& context) override;
};
