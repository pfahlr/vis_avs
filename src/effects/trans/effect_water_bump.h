#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "avs/core/IEffect.hpp"

namespace avs::effects::trans {

/**
 * @brief Combined water ripple and bump mapping distortion.
 */
class WaterBump : public avs::core::IEffect {
 public:
  WaterBump() = default;
  ~WaterBump() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  void resetBuffers(std::size_t pixelCount);
  void applySineBlob(avs::core::RenderContext& context, int x, int y, int radius, int height);
  void calcWater(int nextPage);
  static int clampToRange(int value, int minValue, int maxValue);

  bool enabled_ = true;
  int density_ = 6;
  int depth_ = 600;
  bool randomDrop_ = false;
  int dropPositionX_ = 1;
  int dropPositionY_ = 1;
  int dropRadiusPercent_ = 40;

  int activePage_ = 0;
  int width_ = 0;
  int height_ = 0;
  std::array<std::vector<int>, 2> heightBuffers_{};
  std::vector<std::uint8_t> sourceBuffer_;
};

}  // namespace avs::effects::trans
