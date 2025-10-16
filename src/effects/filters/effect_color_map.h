#pragma once

#include <array>
#include <cstdint>
#include <string_view>

#include "avs/core/IEffect.hpp"

namespace avs::effects::filters {

class ColorMap : public avs::core::IEffect {
 public:
  ColorMap();
  ~ColorMap() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

  enum class Channel { Luma, Red, Green, Blue, Alpha };

 private:
  void parseTable(std::string_view tableText);
  std::uint8_t indexFromPixel(const std::uint8_t* pixel) const;

  std::array<std::array<std::uint8_t, 4>, 256> table_{};
  Channel channel_ = Channel::Luma;
  bool mapAlpha_ = false;
  bool invert_ = false;
};

}  // namespace avs::effects::filters

