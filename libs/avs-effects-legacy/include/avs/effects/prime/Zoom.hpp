#pragma once

#include <avs/core/IEffect.hpp>

namespace avs::effects {

/**
 * @brief Placeholder zoom effect that currently forwards parameters.
 */
class Zoom : public avs::core::IEffect {
 public:
  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  float factor_ = 1.0f;
};

}  // namespace avs::effects
