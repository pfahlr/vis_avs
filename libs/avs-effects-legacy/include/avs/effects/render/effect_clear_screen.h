#pragma once

#include <cstdint>

#include <avs/core/IEffect.hpp>
#include <avs/core/ParamBlock.hpp>
#include <avs/core/RenderContext.hpp>

namespace avs::effects::render {

/**
 * @brief Clear Screen - Fills framebuffer with solid color.
 *
 * Clears the entire framebuffer to a specified color. Supports blend modes
 * for compositing with existing content.
 */
class ClearScreenEffect : public avs::core::IEffect {
 public:
  ClearScreenEffect();
  ~ClearScreenEffect() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

  void setColor(std::uint32_t color) { color_ = color; }
  std::uint32_t color() const { return color_; }

  void setBlendMode(int mode) { blendMode_ = mode; }
  int blendMode() const { return blendMode_; }

 private:
  std::uint32_t color_ = 0x000000;  // RGB color (black by default)
  int blendMode_ = 0;                // 0 = replace, 1 = additive, etc.
};

}  // namespace avs::effects::render
