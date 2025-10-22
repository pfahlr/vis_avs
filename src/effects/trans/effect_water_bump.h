#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "avs/core/IEffect.hpp"

namespace avs::effects::trans {

/**
 * @brief Water ripple effect combined with bump mapped shading.
 */
class WaterBump : public avs::core::IEffect {
 public:
  WaterBump() = default;
  ~WaterBump() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  void ensureBuffers(int width, int height);
  void sineBlob(int x, int y, int radius, int height);
  void calcWater(int destinationPage);
  void applyDisplacementAndLighting(const std::uint8_t* src, std::uint8_t* dst) const;

  static constexpr std::size_t kChannels = 4u;

  bool enabled_ = true;
  int densityShift_ = 6;
  int dropDepth_ = 600;
  bool randomDrop_ = false;
  int dropPositionX_ = 1;
  int dropPositionY_ = 1;
  int dropRadiusPercent_ = 40;

  int width_ = 0;
  int height_ = 0;
  int activePage_ = 0;

  std::array<std::vector<std::int32_t>, 2> heightBuffers_{};
  std::vector<std::uint8_t> scratch_;
};

}  // namespace avs::effects::trans
