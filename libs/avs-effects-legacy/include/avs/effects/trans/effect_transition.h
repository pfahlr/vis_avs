#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <avs/core/IEffect.hpp>
#include <avs/core/ParamBlock.hpp>
#include <avs/core/RenderContext.hpp>

namespace avs::effects::trans {

/**
 * @brief Transition effect with 15 animation modes.
 *
 * Animates between frames using various transition patterns inspired by
 * the original AVS preset transition system. Useful for creating animated
 * effects within a single preset.
 */
class TransitionEffect : public avs::core::IEffect {
 public:
  enum class Mode : std::uint8_t {
    kRandom = 0,
    kCrossDissolve,
    kLeftRightPush,
    kRightLeftPush,
    kTopBottomPush,
    kBottomTopPush,
    kNineRandomBlocks,
    kSplitLeftRightPush,
    kLeftRightToCenterPush,
    kLeftRightToCenterSqueeze,
    kLeftRightWipe,
    kRightLeftWipe,
    kTopBottomWipe,
    kBottomTopWipe,
    kDotDissolve,
  };

  TransitionEffect();
  ~TransitionEffect() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

  void setMode(Mode mode) { mode_ = mode; }
  Mode mode() const { return mode_; }

  void setTransitionSpeed(float speed) { transitionSpeed_ = speed; }
  float transitionSpeed() const { return transitionSpeed_; }

  void setEnabled(bool enabled);
  bool enabled() const { return enabled_; }

 private:
  static constexpr std::size_t kModeCount = 15;

  void prepareBuffers(const avs::core::RenderContext& context);
  void renderCrossDissolve(avs::core::RenderContext& context, float t);
  void renderLeftRightPush(avs::core::RenderContext& context, float t);
  void renderRightLeftPush(avs::core::RenderContext& context, float t);
  void renderTopBottomPush(avs::core::RenderContext& context, float t);
  void renderBottomTopPush(avs::core::RenderContext& context, float t);
  void renderNineRandomBlocks(avs::core::RenderContext& context, float t);
  void renderSplitLeftRightPush(avs::core::RenderContext& context, float t);
  void renderLeftRightToCenterPush(avs::core::RenderContext& context, float t);
  void renderLeftRightToCenterSqueeze(avs::core::RenderContext& context, float t);
  void renderLeftRightWipe(avs::core::RenderContext& context, float t);
  void renderRightLeftWipe(avs::core::RenderContext& context, float t);
  void renderTopBottomWipe(avs::core::RenderContext& context, float t);
  void renderBottomTopWipe(avs::core::RenderContext& context, float t);
  void renderDotDissolve(avs::core::RenderContext& context, float t);

  // Blend two colors with linear interpolation
  static std::array<std::uint8_t, 4> blendColors(
      const std::array<std::uint8_t, 4>& a,
      const std::array<std::uint8_t, 4>& b,
      float t);

  // Apply smooth easing curve
  static float smoothCurve(float t);

  Mode mode_ = Mode::kRandom;
  float transitionSpeed_ = 1.0f;  // Multiplier for transition duration
  bool enabled_ = false;
  double transitionStartTime_ = 0.0;
  std::uint32_t blockMask_ = 0;  // For 9 Random Blocks mode

  // Double buffer for transition animations
  std::vector<std::uint8_t> bufferA_;
  std::vector<std::uint8_t> bufferB_;
  int bufferWidth_ = 0;
  int bufferHeight_ = 0;
  bool buffersValid_ = false;
};

}  // namespace avs::effects::trans
