#pragma once

#include <array>
#include <cstdint>
#include <string>

#include <avs/core/IEffect.hpp>

namespace avs::effects {

enum class SwizzleMode {
  RGB,
  RBG,
  GRB,
  GBR,
  BRG,
  BGR,
};

class Swizzle : public avs::core::IEffect {
 public:
  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  SwizzleMode mode_ = SwizzleMode::RGB;
  std::array<std::uint8_t, 3> order_{0, 1, 2};
};

}  // namespace avs::effects
