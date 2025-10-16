#pragma once

#include <cstdint>

#include "avs/core/IEffect.hpp"

namespace avs::effects::filters {

class FastBrightness : public avs::core::IEffect {
 public:
  FastBrightness() = default;
  ~FastBrightness() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  float amount_ = 2.0f;
  float bias_ = 0.0f;
  bool clampOutput_ = true;
};

}  // namespace avs::effects::filters

