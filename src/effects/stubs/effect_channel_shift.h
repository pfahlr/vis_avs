#pragma once

#include "effects/effect.h"

class Effect_ChannelShift : public Effect {
 public:
  Effect_ChannelShift();
  ~Effect_ChannelShift() override = default;

  bool render(avs::core::RenderContext& context) override;
};
