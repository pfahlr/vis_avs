#pragma once

#include <avs/effects/legacy/dynamic/frame_warp.h>

namespace avs::effects {

class MovementEffect : public FrameWarpEffect {
 public:
  MovementEffect() = default;
  ~MovementEffect() override = default;

  void setParams(const avs::core::ParamBlock& params) override;
  bool render(avs::core::RenderContext& context) override;

 private:
  float scale_{1.0f};
  float rotationDeg_{0.0f};
  float offsetX_{0.0f};
  float offsetY_{0.0f};
  bool wrap_{false};
};

}  // namespace avs::effects

