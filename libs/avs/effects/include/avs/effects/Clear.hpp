#pragma once

#include <cstdint>

#include "avs/core/IEffect.hpp"

namespace avs::effects {

/**
 * @brief Fills the framebuffer with a constant color value.
 */
class Clear : public avs::core::IEffect {
 public:
  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  std::uint8_t value_ = 0;
};

}  // namespace avs::effects
