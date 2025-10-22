#pragma once

#include <string>
#include <vector>

#include <avs/core/IEffect.hpp>

namespace avs::effects {

class Bump : public avs::core::IEffect {
 public:
  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  float scaleX_ = 0.0f;
  float scaleY_ = 0.0f;
  float midpoint_ = 0.5f;
  bool useFrameHeightmap_ = true;
  std::string heightmapKey_;
};

}  // namespace avs::effects

