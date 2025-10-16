#pragma once

#include "effects/effect.h"

class Effect_MiscComment : public Effect {
 public:
  Effect_MiscComment();
  ~Effect_MiscComment() override = default;

  bool render(avs::core::RenderContext& context) override;
};
