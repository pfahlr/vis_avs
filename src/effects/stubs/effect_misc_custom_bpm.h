#pragma once

#include "effects/effect.h"

class Effect_MiscCustomBpm : public Effect {
 public:
  Effect_MiscCustomBpm();
  ~Effect_MiscCustomBpm() override = default;

  bool render(avs::core::RenderContext& context) override;
};
