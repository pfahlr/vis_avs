#pragma once

#include "effects/effect.h"

class Effect_TransBrightness : public Effect {
 public:
  Effect_TransBrightness();
  ~Effect_TransBrightness() override = default;

  bool render(avs::core::RenderContext& context) override;
};
