#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "avs/core/IEffect.hpp"

namespace avs::effects::trans {

class WaterBump : public avs::core::IEffect {
 public:
  WaterBump() = default;
  ~WaterBump() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  static constexpr std::size_t kChannels = 4u;

  void ensureBuffers(std::size_t width, std::size_t height);
  void resetBuffers(std::size_t totalPixels);
  void spawnDrop(avs::core::RenderContext& context);
  void applySineBlob(int centerX, int centerY, int radius, int heightDelta);
  void applyHeightBlob(int centerX, int centerY, int radius, int heightDelta);
  void renderFrame(const std::uint8_t* srcPixels);
  void updateWater(int nextPage);

  bool enabled_ = true;
  int density_ = 6;
  int depth_ = 600;
  bool randomDrop_ = false;
  int dropPosX_ = 1;
  int dropPosY_ = 1;
  int dropRadiusPercent_ = 40;
  int method_ = 0;

  int width_ = 0;
  int height_ = 0;
  int currentPage_ = 0;

  std::array<std::vector<int>, 2> heightBuffers_{};
  std::vector<std::uint8_t> scratch_;
};

}  // namespace avs::effects::trans
