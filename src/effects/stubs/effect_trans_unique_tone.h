#pragma once

#include "effects/effect.h"

class Effect_TransUniqueTone : public Effect {
 public:
  Effect_TransUniqueTone();
  ~Effect_TransUniqueTone() override = default;

  bool render(avs::core::RenderContext& context) override;
};
