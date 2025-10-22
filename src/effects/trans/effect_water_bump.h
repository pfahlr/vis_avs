#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "avs/core/DeterministicRng.hpp"
#include "avs/core/IEffect.hpp"

namespace avs::effects::trans {

/**
 * @brief Water ripple simulation combined with bump-mapped framebuffer sampling.
 */
class WaterBump : public avs::core::IEffect {
 public:
  WaterBump() = default;
  ~WaterBump() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  using HeightBuffer = std::vector<int>;

  bool ensureBuffers(int width, int height);
  void sineBlob(int x,
                int y,
                int radius,
                int height,
                avs::core::DeterministicRng& rng);
  void calcWater(int nextPage);
  static int clampDensity(int value);
  static int randomInRange(avs::core::DeterministicRng& rng, int minInclusive, int maxInclusive);

  bool enabled_ = true;
  int density_ = 6;
  int depth_ = 600;
  bool randomDrop_ = false;
  int dropPositionX_ = 1;
  int dropPositionY_ = 1;
  int dropRadius_ = 40;
  [[maybe_unused]] int method_ = 0;

  int width_ = 0;
  int height_ = 0;
  int page_ = 0;
  std::array<HeightBuffer, 2> heightMaps_;
  std::vector<std::uint8_t> sourceFrame_;
};

}  // namespace avs::effects::trans

