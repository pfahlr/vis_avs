#pragma once

#include <avs/effects/legacy/effect.h>

class Effect_RenderAvi : public Effect {
 public:
  Effect_RenderAvi();
  ~Effect_RenderAvi() override = default;

  bool render(avs::core::RenderContext& context) override;
};
