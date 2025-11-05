#pragma once

#include <cstdint>
#include <vector>

#include "avs/effects/legacy_effect.h"

namespace avs::effects::trans {

class EffectTransition : public LegacyEffect {
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

  EffectTransition() = default;

  void render(LegacyRenderContext& context) override;
  void loadConfig(const std::uint8_t* data, std::size_t size) override;
  std::vector<std::uint8_t> saveConfig() const override;

  void setMode(Mode mode) { mode_ = mode; }
  Mode mode() const { return mode_; }

 private:
  static constexpr std::size_t kModeCount = 15;
  Mode mode_ = Mode::kRandom;
};

}  // namespace avs::effects::trans
