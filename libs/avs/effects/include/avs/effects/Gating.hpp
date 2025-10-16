#pragma once

namespace avs::effects {

enum class GateFlag {
  Off,
  Beat,
  Hold,
  Sticky,
};

struct GateResult {
  bool render{true};
  GateFlag flag{GateFlag::Off};
};

struct GateOptions {
  bool enableOnBeat{false};
  bool stickyToggle{false};
  bool onlySticky{false};
  int holdFrames{2};
};

class BeatGate {
 public:
  BeatGate() = default;

  void configure(const GateOptions& options);
  void reset();
  [[nodiscard]] GateResult step(bool beatEvent);
  [[nodiscard]] bool stickyActive() const { return stickyActive_; }

 private:
  GateOptions options_{};
  int holdCounter_{0};
  bool stickyActive_{false};
};

}  // namespace avs::effects
