#include <avs/effects/core/gating.h>

namespace avs::effects {

void BeatGate::configure(const GateOptions& options) {
  options_ = options;
  holdCounter_ = 0;
  stickyActive_ = false;
}

void BeatGate::reset() {
  holdCounter_ = 0;
  stickyActive_ = false;
}

GateResult BeatGate::step(bool beatEvent) {
  GateResult result{};
  if (!options_.enableOnBeat) {
    result.render = true;
    result.flag = GateFlag::Hold;
    return result;
  }

  bool beatTriggered = false;
  if (beatEvent) {
    beatTriggered = true;
    holdCounter_ = options_.holdFrames;
    if (options_.stickyToggle) {
      stickyActive_ = !stickyActive_;
    }
    result.flag = GateFlag::Beat;
  }

  if (!beatTriggered && holdCounter_ > 0 && !(options_.stickyToggle && stickyActive_)) {
    --holdCounter_;
  }

  if (options_.stickyToggle) {
    if (stickyActive_) {
      result.render = true;
      result.flag = GateFlag::Sticky;
      return result;
    }

    if (options_.onlySticky) {
      if (!beatTriggered) {
        result.render = stickyActive_;
        if (!stickyActive_) {
          result.flag = GateFlag::Off;
        }
        return result;
      }

      result.render = stickyActive_;
      if (stickyActive_) {
        result.flag = GateFlag::Sticky;
      }
      return result;
    }

    if (beatTriggered) {
      result.render = true;
      return result;
    }

    if (holdCounter_ > 0) {
      result.render = true;
      result.flag = GateFlag::Hold;
      return result;
    }

    result.render = false;
    result.flag = GateFlag::Off;
    return result;
  }

  if (beatTriggered) {
    result.render = true;
    return result;
  }

  if (holdCounter_ > 0) {
    result.render = true;
    result.flag = GateFlag::Hold;
    return result;
  }

  result.render = false;
  result.flag = GateFlag::Off;
  return result;
}

}  // namespace avs::effects
