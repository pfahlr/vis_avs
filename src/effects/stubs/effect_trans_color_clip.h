#pragma once

#include "effects/effect.h"

class Effect_TransColorClip : public Effect {
 public:
  Effect_TransColorClip();
  ~Effect_TransColorClip() override = default;

  bool render(avs::core::RenderContext& context) override;
};
