#pragma once

#include <avs/core/IEffect.hpp>
#include <avs/core/ParamBlock.hpp>
#include <avs/core/RenderContext.hpp>

namespace avs::effects::trans {

/**
 * @brief Invert - Simple per-pixel color inversion effect.
 *
 * Inverts the RGB color values of each pixel in the framebuffer.
 * This is a simple transformation: new_color = 255 - old_color
 */
class InvertEffect : public avs::core::IEffect {
 public:
  InvertEffect() = default;
  ~InvertEffect() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

  void setEnabled(bool enabled) { enabled_ = enabled; }
  bool enabled() const { return enabled_; }

 private:
  bool enabled_ = true;
};

}  // namespace avs::effects::trans
