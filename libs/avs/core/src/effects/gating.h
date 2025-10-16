#pragma once

#include <cstdint>
#include <vector>

#include "avs/core/IEffect.hpp"

namespace avs::core {

/**
 * @brief Beat-aware gating effect that toggles downstream rendering.
 */
class BeatGatingEffect : public IEffect {
 public:
  struct DebugState {
    bool active{true};
    bool latched{false};
    bool triggered{false};
    std::uint8_t flag{0};
  };

  BeatGatingEffect() = default;

  bool render(RenderContext& context) override;
  void setParams(const ParamBlock& params) override;

  [[nodiscard]] DebugState debugState() const { return {active_, latched_, lastTriggered_, lastFlag_}; }

 private:
  void appendHistory(std::uint8_t flag);
  void drawHistory(RenderContext& context) const;
  std::uint8_t stateToFlag(bool triggered, bool beatRejected) const;

  bool onBeat_{false};
  bool stick_{false};
  bool randomPosition_{false};
  bool fiftyFifty_{false};
  bool onlySticky_{false};
  int logHeight_{6};
  mutable std::vector<std::uint8_t> history_;
  bool active_{true};
  bool latched_{false};
  bool lastTriggered_{false};
  std::uint8_t lastFlag_{0};
  float offsetX_{0.0f};
  float offsetY_{0.0f};
};

}  // namespace avs::core
