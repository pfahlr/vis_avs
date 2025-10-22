#pragma once

#include <array>
#include <cstdint>

#include <avs/core/IEffect.hpp>

namespace avs::effects::trans {

/**
 * @brief Legacy-compatible brightness transformer matching r_bright.cpp.
 */
class Brightness : public avs::core::IEffect {
 public:
  Brightness() = default;
  ~Brightness() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  void updateLookupTables();
  [[nodiscard]] bool shouldSkipPixel(const std::uint8_t* pixel) const;

  static int computeMultiplier(int sliderValue);
  static std::uint8_t applyMultiplier(std::uint8_t value, int multiplier);
  static std::uint8_t saturatingAdd(std::uint8_t base, std::uint8_t addend);

  bool enabled_ = true;
  bool blendAdditive_ = false;
  bool blendAverage_ = true;
  bool exclude_ = false;
  int distance_ = 16;
  std::uint32_t referenceColor_ = 0u;
  std::uint8_t referenceRed_ = 0u;
  std::uint8_t referenceGreen_ = 0u;
  std::uint8_t referenceBlue_ = 0u;
  int redSlider_ = 0;
  int greenSlider_ = 0;
  int blueSlider_ = 0;
  bool tablesDirty_ = true;
  std::array<std::uint8_t, 256> redTable_{};
  std::array<std::uint8_t, 256> greenTable_{};
  std::array<std::uint8_t, 256> blueTable_{};
};

}  // namespace avs::effects::trans
