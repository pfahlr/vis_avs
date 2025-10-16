#pragma once

#include "effects/effect.h"

class Effect_RenderOscilloscopeStar : public Effect {
 public:
  Effect_RenderOscilloscopeStar();
  ~Effect_RenderOscilloscopeStar() override = default;

  bool render(avs::core::RenderContext& context) override;
};
