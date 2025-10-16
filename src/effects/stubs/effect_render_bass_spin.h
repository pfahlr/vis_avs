#pragma once

#include "effects/effect.h"

class Effect_RenderBassSpin : public Effect {
 public:
  Effect_RenderBassSpin();
  ~Effect_RenderBassSpin() override = default;

  bool render(avs::core::RenderContext& context) override;
};
