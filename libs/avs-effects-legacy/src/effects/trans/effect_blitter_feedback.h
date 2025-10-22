#pragma once

#include <cstdint>
#include <string>

#include "effects/dynamic/frame_warp.h"

namespace avs::effects::trans {

/**
 * @brief Reprojects the previous frame with optional mirroring and rotation.
 *
 * The effect samples the stored history framebuffer using the legacy AVS
 * coordinate space (\f$[-1, 1]\f$) and writes the transformed image back into
 * the current framebuffer. A feedback gain can be applied to keep the feedback
 * loop bounded.
 */
class BlitterFeedback : public avs::effects::FrameWarpEffect {
 public:
  BlitterFeedback() = default;
  ~BlitterFeedback() override = default;

  void setParams(const avs::core::ParamBlock& params) override;
  bool render(avs::core::RenderContext& context) override;

 private:
  static int normalizeQuadrants(int value);
  static void applyModeString(const std::string& mode, bool& mirrorX, bool& mirrorY, int& rotateQuadrants);
  [[nodiscard]] std::uint8_t applyFeedback(std::uint8_t value) const;

  bool mirrorX_ = false;
  bool mirrorY_ = false;
  int rotateQuadrants_ = 0;
  bool wrap_ = false;
  float feedback_ = 1.0f;
};

}  // namespace avs::effects::trans

