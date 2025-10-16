#pragma once

#include "effects/effect.h"

class Effect_RenderDotPlane : public Effect {
 public:
  Effect_RenderDotPlane();
  ~Effect_RenderDotPlane() override = default;

  bool render(avs::core::RenderContext& context) override;
};
