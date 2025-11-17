#pragma once

#include <avs/effects/dynamic/frame_warp.h>

namespace avs::effects {

class ZoomRotateEffect : public FrameWarpEffect {
 public:
  ZoomRotateEffect() = default;
  ~ZoomRotateEffect() override = default;

  void setParams(const avs::core::ParamBlock& params) override;
  bool render(avs::core::RenderContext& context) override;

 private:
  float zoom_{1.0f};
  float rotationDeg_{0.0f};
  float anchorX_{0.5f};
  float anchorY_{0.5f};
  bool wrap_{false};
};

}  // namespace avs::effects

