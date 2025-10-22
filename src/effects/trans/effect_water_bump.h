#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "avs/core/IEffect.hpp"

namespace avs::effects::trans {

class WaterBump : public avs::core::IEffect {
 public:
  WaterBump();
  ~WaterBump() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  enum class DropAxisPosition : int {
    Left = 0,
    Center = 1,
    Right = 2,
  };

  void ensureCapacity(std::size_t width, std::size_t height);
  void addDrop(const avs::core::RenderContext& context);
  void addSineBlob(int centerX, int centerY, float radius, float amplitude);
  void propagateWave();
  void applyWaterBump(avs::core::RenderContext& context);

  [[nodiscard]] float computeRadiusPixels() const;
  [[nodiscard]] int computeDropCoordinate(DropAxisPosition position, int extent) const;
  static DropAxisPosition clampDropPosition(int value);
  static std::uint8_t clampByte(int value);

  bool enabled_ = true;
  int densityShift_ = 6;
  int depth_ = 600;
  bool randomDrop_ = false;
  DropAxisPosition dropPositionX_ = DropAxisPosition::Center;
  DropAxisPosition dropPositionY_ = DropAxisPosition::Center;
  int dropRadiusPercent_ = 40;
  int method_ = 0;

  int width_ = 0;
  int height_ = 0;
  std::array<std::vector<float>, 2> heightBuffers_{};
  int frontBuffer_ = 0;
  std::vector<std::uint8_t> scratch_;
};

}  // namespace avs::effects::trans
