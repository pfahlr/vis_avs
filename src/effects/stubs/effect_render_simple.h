#pragma once

#include "effects/effect.h"

class Effect_RenderSimple : public Effect {
 public:
  Effect_RenderSimple();
  ~Effect_RenderSimple() override = default;

  bool render(avs::core::RenderContext& context) override;
};
