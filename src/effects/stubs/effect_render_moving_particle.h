#pragma once

#include "effects/effect.h"

class Effect_RenderMovingParticle : public Effect {
 public:
  Effect_RenderMovingParticle();
  ~Effect_RenderMovingParticle() override = default;

  bool render(avs::core::RenderContext& context) override;
};
