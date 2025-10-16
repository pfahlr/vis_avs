#pragma once

#include "effects/effect.h"

class Effect_RenderRotatingStars : public Effect {
 public:
  Effect_RenderRotatingStars();
  ~Effect_RenderRotatingStars() override = default;

  bool render(avs::core::RenderContext& context) override;
};
