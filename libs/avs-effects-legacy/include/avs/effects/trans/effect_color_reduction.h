#pragma once

#include <cstdint>

#include <avs/core/IEffect.hpp>
#include <avs/core/ParamBlock.hpp>

namespace avs::effects::trans {

/**
 * @brief Matches the legacy "Color Reduction" transform by masking the low bits of each RGB channel.
 */
class ColorReduction : public avs::core::IEffect {
 public:
  ColorReduction();
  ~ColorReduction() override = default;

  void setParams(const avs::core::ParamBlock& params) override;
  bool render(avs::core::RenderContext& context) override;

 private:
  void updateMask();

  int levels_ = 7;
  std::uint8_t channelMask_ = 0xFEu;
};

}  // namespace avs::effects::trans

