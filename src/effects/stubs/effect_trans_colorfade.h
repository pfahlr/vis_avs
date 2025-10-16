#pragma once

#include "effects/effect.h"

class Effect_TransColorfade : public Effect {
 public:
  Effect_TransColorfade();
  ~Effect_TransColorfade() override = default;

  bool render(avs::core::RenderContext& context) override;
};
