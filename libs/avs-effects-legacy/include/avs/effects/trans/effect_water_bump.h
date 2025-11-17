#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include <avs/core/IEffect.hpp>

namespace avs::core {
class DeterministicRng;
}

namespace avs::effects::trans {

/**
 * @brief Recreates the legacy "Trans / Water Bump" effect.
 *
 * The effect maintains a simulated heightmap to both displace the current
 * framebuffer and propagate ripples over time, closely matching the behaviour
 * of the original Winamp AVS module.
 */
class WaterBump : public avs::core::IEffect {
 public:
  WaterBump() = default;
  ~WaterBump() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  static constexpr std::size_t kChannels = 4u;

  bool ensureResources(int width, int height, std::size_t requiredBytes);
  void applyDrop(avs::core::RenderContext& context);
  void applySineBlob(int x, int y, int radius, int heightDelta, avs::core::DeterministicRng& rng);
  void applyHeightBlob(int x, int y, int radius, int heightDelta, avs::core::DeterministicRng& rng);
  void simulateWater();

  bool enabled_ = true;
  int density_ = 6;
  int depth_ = 600;
  bool randomDrop_ = false;
  int dropPositionX_ = 1;
  int dropPositionY_ = 1;
  int dropRadius_ = 40;
  int method_ = 0;

  int bufferWidth_ = 0;
  int bufferHeight_ = 0;
  int currentPage_ = 0;
  std::vector<int> heightBuffers_[2];
  std::vector<std::uint8_t> scratch_;
};

}  // namespace avs::effects::trans
