#pragma once

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
  void ensureBuffers(int width, int height);
  void spawnBeatDrop(avs::core::RenderContext& context);
  void applySineBlob(int x, int y, int radius, int height, avs::core::DeterministicRng* rng);
  void simulate(int sourcePage, int destinationPage);
  void simulateSludge(int sourcePage, int destinationPage);
  void displaceFrame(const std::uint32_t* source, std::uint32_t* destination, int width,
                     int height) const;

  static bool hasFramebuffer(const avs::core::RenderContext& context);
  static int clamp(int value, int minValue, int maxValue);

  bool enabled_ = true;
  int density_ = 6;
  int depth_ = 600;
  bool randomDrop_ = false;
  int dropPositionX_ = 1;
  int dropPositionY_ = 1;
  int dropRadius_ = 40;
  int method_ = 0;

  int page_ = 0;
  int width_ = 0;
  int height_ = 0;

  std::vector<int> heightBuffers_[2];
  std::vector<std::uint8_t> scratch_;
};

}  // namespace avs::effects::trans
