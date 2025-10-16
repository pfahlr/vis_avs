#pragma once

#include "effects/effect.h"

class Effect_RenderTimescope : public Effect {
 public:
  Effect_RenderTimescope();
  ~Effect_RenderTimescope() override = default;

  bool render(avs::core::RenderContext& context) override;
};
