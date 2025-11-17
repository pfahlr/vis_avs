#include "avs/effects/trans/effect_transition.h"

#include "avs/effects/effect_registry.hpp"

namespace avs::effects::trans {


namespace {
constexpr std::uint8_t kMaxModeIndex = static_cast<std::uint8_t>(EffectTransition::Mode::kDotDissolve);
}

void EffectTransition::render(LegacyRenderContext& context) {
  (void)context;
  // TODO: Implement legacy transition rendering modes.
  switch (mode_) {
    case Mode::kRandom:
    case Mode::kCrossDissolve:
    case Mode::kLeftRightPush:
    case Mode::kRightLeftPush:
    case Mode::kTopBottomPush:
    case Mode::kBottomTopPush:
    case Mode::kNineRandomBlocks:
    case Mode::kSplitLeftRightPush:
    case Mode::kLeftRightToCenterPush:
    case Mode::kLeftRightToCenterSqueeze:
    case Mode::kLeftRightWipe:
    case Mode::kRightLeftWipe:
    case Mode::kTopBottomWipe:
    case Mode::kBottomTopWipe:
    case Mode::kDotDissolve:
      break;
  }
}

void EffectTransition::loadConfig(const std::uint8_t* data, std::size_t size) {
  if (data == nullptr || size == 0) {
    mode_ = Mode::kRandom;
    return;
  }
  std::uint8_t index = data[0];
  if (index > kMaxModeIndex) {
    index = 0;
  }
  mode_ = static_cast<Mode>(index);
}

std::vector<std::uint8_t> EffectTransition::saveConfig() const {
  return {static_cast<std::uint8_t>(mode_)};
}

}  // namespace avs::effects::trans
