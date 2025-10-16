#pragma once

#include "effects/effect.h"

class Effect_RenderRing : public Effect {
 public:
  Effect_RenderRing();
  ~Effect_RenderRing() override = default;

  bool render(avs::core::RenderContext& context) override;
};
