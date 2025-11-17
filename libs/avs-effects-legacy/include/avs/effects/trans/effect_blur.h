#pragma once

#include <cstdint>
#include <vector>

#include <avs/core/IEffect.hpp>

namespace avs::effects::trans {

class R_Blur : public avs::core::IEffect {
 public:
  R_Blur() = default;
  ~R_Blur() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  void ensureBuffers(int width, int height);
  bool renderBox(avs::core::RenderContext& context);
  void horizontalPass(const std::uint8_t* src, std::uint8_t* dst, int width, int height) const;
  void verticalPass(const std::uint8_t* src, std::uint8_t* dst, int width, int height) const;
  void blend(std::uint8_t* dst,
             const std::uint8_t* original,
             const std::uint8_t* blurred,
             int width,
             int height) const;

  static int clampIndex(int value, int minValue, int maxValue);
  static std::uint8_t clampByte(int value);

  int radius_ = 1;
  int strength_ = 256;
  bool horizontal_ = true;
  bool vertical_ = true;
  bool roundMode_ = false;

  mutable std::vector<std::uint8_t> original_;
  mutable std::vector<std::uint8_t> temp_;
  mutable std::vector<std::uint8_t> blurred_;
  mutable std::vector<int> prefixRow_;
  mutable std::vector<int> prefixColumn_;
};

}  // namespace avs::effects::trans

