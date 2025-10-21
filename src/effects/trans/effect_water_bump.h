#pragma once

#include <array>
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
  void resetBuffers(int width, int height);
  void applyBeatDrop(avs::core::RenderContext& context);
  void addSineBlob(int centerX, int centerY, int radius, int amplitude);
  void updateWater(int nextPage);
  [[nodiscard]] bool hasFramebuffer(const avs::core::RenderContext& context) const;
  [[nodiscard]] static int clampCenter(int value, int radius, int dimension);
  [[nodiscard]] static int randomInRange(avs::core::DeterministicRng& rng, int minInclusive,
                                         int maxInclusive);

  bool enabled_ = true;
  int damping_ = 6;
  int depth_ = 600;
  bool randomDrop_ = false;
  int dropPosX_ = 1;
  int dropPosY_ = 1;
  int dropRadius_ = 40;
  int method_ = 0;

  int bufferWidth_ = 0;
  int bufferHeight_ = 0;
  int currentPage_ = 0;
  std::array<std::vector<int>, 2> heightBuffers_{};
  std::vector<std::uint8_t> scratch_;
};

}  // namespace avs::effects::trans
