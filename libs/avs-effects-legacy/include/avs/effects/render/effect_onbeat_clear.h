#pragma once

#include <cstdint>

#include <avs/core/IEffect.hpp>
#include <avs/core/ParamBlock.hpp>
#include <avs/core/RenderContext.hpp>

namespace avs::effects::render {

/**
 * @brief OnBeat Clear - Clears framebuffer when beat is detected.
 *
 * Monitors beat detection and clears the screen to a specified color
 * when a beat occurs. Useful for creating synchronized visual effects.
 */
class OnBeatClearEffect : public avs::core::IEffect {
 public:
  OnBeatClearEffect();
  ~OnBeatClearEffect() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

  void setColor(std::uint32_t color) { color_ = color; }
  std::uint32_t color() const { return color_; }

  void setEnabled(bool enabled) { enabled_ = enabled; }
  bool enabled() const { return enabled_; }

 private:
  std::uint32_t color_ = 0x000000;  // RGB color (black by default)
  bool enabled_ = true;
};

}  // namespace avs::effects::render
