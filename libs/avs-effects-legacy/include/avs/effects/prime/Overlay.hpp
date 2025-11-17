#pragma once

#include <array>
#include <cstdint>

#include <avs/core/IEffect.hpp>
#include <avs/effects/core/blend_ops.hpp>

namespace avs::effects {

class Overlay : public avs::core::IEffect {
 public:
  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  BlendOp op_ = BlendOp::Replace;
  std::array<std::uint8_t, 4> background_{0, 0, 0, 255};
  std::array<std::uint8_t, 4> foreground_{0, 0, 0, 255};
  std::uint8_t alpha_ = 255;
  std::uint8_t alpha2_ = 255;
  std::uint8_t slide_ = 255;
};

}  // namespace avs::effects
