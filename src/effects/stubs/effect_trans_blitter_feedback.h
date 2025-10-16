#pragma once

#include "effects/effect.h"

class Effect_TransBlitterFeedback : public Effect {
 public:
  Effect_TransBlitterFeedback();
  ~Effect_TransBlitterFeedback() override = default;

  bool render(avs::core::RenderContext& context) override;
};
