#pragma once

#include <array>
#include <cstdint>

#include <avs/core/IEffect.hpp>
#include <avs/core/ParamBlock.hpp>

namespace avs::effects::trans {

class ColorClip : public avs::core::IEffect {
 public:
  ColorClip() = default;
  ~ColorClip() override = default;

  void setParams(const avs::core::ParamBlock& params) override;
  bool render(avs::core::RenderContext& context) override;

 private:
  bool enabled_ = true;
  std::array<std::uint8_t, 3> clipColor_{32u, 32u, 32u};
};

}  // namespace avs::effects::trans

