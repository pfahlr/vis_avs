#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "avs/core/IEffect.hpp"

namespace avs::effects {

// Helper base for transform-style effects that need access to the previous
// frame. The class maintains an internal copy of the last rendered frame and
// offers bilinear sampling in the normalized AVS coordinate space
// (x,y in [-1, 1]).
class FrameWarpEffect : public avs::core::IEffect {
 public:
  FrameWarpEffect() = default;
  ~FrameWarpEffect() override = default;

 protected:
  using Rgba = std::array<std::uint8_t, 4>;

  // Ensures the internal history buffer matches the provided framebuffer size.
  // On the very first call the current framebuffer contents become the history
  // so the first render pass samples from the supplied pixels.
  bool prepareHistory(const avs::core::RenderContext& context);

  // Copies the freshly rendered frame into the history buffer so the next
  // invocation can sample from it. The caller must invoke this after writing
  // the output frame.
  void storeHistory(const avs::core::RenderContext& context);

  // Samples from the history buffer using normalized coordinates. When
  // `wrap` is true the sampling wraps around the framebuffer edges; otherwise
  // it clamps to the valid domain.
  [[nodiscard]] Rgba sampleHistory(float normX, float normY, bool wrap) const;

  [[nodiscard]] int historyWidth() const { return width_; }
  [[nodiscard]] int historyHeight() const { return height_; }

 private:
  [[nodiscard]] Rgba bilinearSample(float fx, float fy, bool wrap) const;

  static int wrapIndex(int value, int size);
  static float wrapCoord(float value, float size);

  std::vector<std::uint8_t> history_;
  int width_{0};
  int height_{0};
};

}  // namespace avs::effects

