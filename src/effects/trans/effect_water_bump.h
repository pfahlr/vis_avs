#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "avs/core/IEffect.hpp"

namespace avs::effects::trans {

/**
 * @brief Combines a water height field simulation with bump-mapped image
 * displacement.
 */
class WaterBump : public avs::core::IEffect {
 public:
  WaterBump() = default;
  ~WaterBump() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  static constexpr std::size_t kChannels = 4u;

  [[nodiscard]] bool hasFramebuffer(const avs::core::RenderContext& context,
                                    std::size_t requiredBytes) const;
  void ensureBuffers(int width, int height);
  void applyBeatDrop(avs::core::RenderContext& context);
  void sineBlob(int x, int y, int radius, int height);
  void calcWater(int targetPage);

  bool enabled_ = true;
  int density_ = 6;
  int depth_ = 600;
  bool randomDrop_ = false;
  int dropPositionX_ = 1;
  int dropPositionY_ = 1;
  int dropRadiusPercent_ = 40;
  int method_ = 0;

  int bufferWidth_ = 0;
  int bufferHeight_ = 0;
  int page_ = 0;
  std::array<std::vector<int>, 2> heightBuffers_{};
  std::vector<std::uint8_t> inputCopy_;
  std::vector<std::uint8_t> scratch_;
};

}  // namespace avs::effects::trans
