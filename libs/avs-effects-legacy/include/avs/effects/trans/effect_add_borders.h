#pragma once

#include <cstdint>
#include <avs/core/IEffect.hpp>

namespace avs::effects::trans {

class AddBorders : public avs::core::IEffect {
 public:
  AddBorders();
  ~AddBorders() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  std::uint32_t color_ = 0x000000;  // Default: black
  int size_ = 10;  // Border size percentage (0-100)
  bool enabled_ = true;

  static std::uint32_t makeRGBA(std::uint32_t rgb);
};

}  // namespace avs::effects::trans
