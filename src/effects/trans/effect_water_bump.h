#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "avs/core/IEffect.hpp"

namespace avs::effects::trans {

/**
 * @brief Combined water displacement and bump mapping transformer.
 */
class WaterBump : public avs::core::IEffect {
 public:
  WaterBump() = default;
  ~WaterBump() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  void ensureState(int width, int height);
  void injectDrop(avs::core::RenderContext& context);
  void addSineBlob(int centerX, int centerY, int radius, int amplitude);
  void calcWater(int nextPage);

  static std::uint8_t clampByte(int value);

  bool enabled_ = true;
  int damping_ = 6;
  int dropDepth_ = 600;
  bool randomDrop_ = false;
  int dropPositionX_ = 1;
  int dropPositionY_ = 1;
  int dropRadiusPercent_ = 40;
  float bumpStrength_ = 1.0f;

  int width_ = 0;
  int height_ = 0;
  int currentPage_ = 0;

  std::array<std::vector<int>, 2> heightBuffers_{};
  std::vector<std::uint8_t> output_;
};

}  // namespace avs::effects::trans
