#pragma once

#include <array>
#include <cstdint>

#include <avs/core/IEffect.hpp>

namespace avs::effects::trans {

class Multiplier : public avs::core::IEffect {
 public:
  Multiplier() = default;
  ~Multiplier() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  enum class Mode {
    kInfinity = 0,
    kX8 = 1,
    kX4 = 2,
    kX2 = 3,
    kHalf = 4,
    kQuarter = 5,
    kEighth = 6,
    kZero = 7,
  };

  static Mode decodeMode(int value) noexcept;
  static bool hasFramebuffer(const avs::core::RenderContext& context) noexcept;
  static std::uint8_t multiplyChannel(std::uint8_t value, int factor) noexcept;
  static std::uint8_t scaleChannel(std::uint8_t value, float factor) noexcept;

  Mode mode_ = Mode::kX2;
  bool useCustomFactors_ = false;
  std::array<float, 3> customFactors_{2.0f, 2.0f, 2.0f};
};

}  // namespace avs::effects::trans
