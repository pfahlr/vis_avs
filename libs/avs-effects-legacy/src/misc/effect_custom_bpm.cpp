#include <avs/effects/misc/effect_custom_bpm.h>

#include <algorithm>
#include <cmath>

#include <avs/core/ParamBlock.hpp>
#include <avs/runtime/GlobalState.hpp>
#include <avs/audio/analyzer.h>

namespace avs::effects::misc {

namespace {
constexpr float kMinBpm = 10.0f;
constexpr float kMaxBpm = 480.0f;
constexpr int kMinSkipInterval = 1;
constexpr int kMaxSkipInterval = 64;
constexpr int kMaxSkipFirst = 64;

int clampRegisterIndex(int value) {
  if (value < 1 || value > static_cast<int>(avs::runtime::GlobalState::kRegisterCount)) {
    return -1;
  }
  return value - 1;
}

}  // namespace

CustomBpmEffect::CustomBpmEffect()
    : enabled_(true),
      mode_(Mode::Arbitrary),
      bpm_(120.0f),
      skipInterval_(kMinSkipInterval + 0),
      skipFirstCount_(0),
      beatsSeen_(0),
      skipCounter_(0),
      accumulatorSeconds_(0.0),
      gateOptions_{},
      gateRenderRegister_(-1),
      gateFlagRegister_(-1) {
  gateOptions_.enableOnBeat = true;
  gateOptions_.holdFrames = 0;
  configureGate();
}

void CustomBpmEffect::resetState() {
  beatsSeen_ = 0;
  skipCounter_ = 0;
  accumulatorSeconds_ = 0.0;
  gate_.reset();
}

void CustomBpmEffect::configureGate() {
  gate_.configure(gateOptions_);
  gate_.reset();
}

double CustomBpmEffect::intervalSeconds() const {
  if (bpm_ <= 0.0f) {
    return 0.0;
  }
  return 60.0 / static_cast<double>(bpm_);
}

void CustomBpmEffect::writeGateRegisters(avs::core::RenderContext& context,
                                         const avs::effects::GateResult& gate) const {
  if (!context.globals) {
    return;
  }
  if (gateRenderRegister_ >= 0) {
    context.globals->registers[static_cast<std::size_t>(gateRenderRegister_)] = gate.render ? 1.0 : 0.0;
  }
  if (gateFlagRegister_ >= 0) {
    context.globals->registers[static_cast<std::size_t>(gateFlagRegister_)] =
        gate.render ? static_cast<double>(gate.flag == avs::effects::GateFlag::Beat   ? 1 :
                                          gate.flag == avs::effects::GateFlag::Hold   ? 2 :
                                          gate.flag == avs::effects::GateFlag::Sticky ? 3 : 0)
                    : 0.0;
  }
}

void CustomBpmEffect::setParams(const avs::core::ParamBlock& params) {
  const bool enabled = params.getBool("enabled", enabled_);
  const bool hasArbitraryParam = params.contains("arbitrary");
  const bool hasSkipParam = params.contains("skip");
  const bool hasInvertParam = params.contains("invert");

  const bool arbitrary = hasArbitraryParam ? params.getBool("arbitrary", mode_ == Mode::Arbitrary)
                                           : (mode_ == Mode::Arbitrary);
  const bool skip = hasSkipParam ? params.getBool("skip", mode_ == Mode::Skip) : (mode_ == Mode::Skip);
  const bool invert = hasInvertParam ? params.getBool("invert", mode_ == Mode::Invert)
                                     : (mode_ == Mode::Invert);

  Mode newMode = mode_;
  if (hasArbitraryParam || hasSkipParam || hasInvertParam) {
    newMode = Mode::Passthrough;
    if (arbitrary) {
      newMode = Mode::Arbitrary;
    } else if (skip) {
      newMode = Mode::Skip;
    } else if (invert) {
      newMode = Mode::Invert;
    }
  }

  float bpm = bpm_;
  if (params.contains("bpm")) {
    bpm = params.getFloat("bpm", bpm_);
  } else if (params.contains("arbval")) {
    const int intervalMs = std::max(1, params.getInt("arbval", 500));
    bpm = 60000.0f / static_cast<float>(intervalMs);
  } else if (params.contains("interval_ms")) {
    const int intervalMs = std::max(1, params.getInt("interval_ms", 500));
    bpm = 60000.0f / static_cast<float>(intervalMs);
  }
  bpm = std::clamp(bpm, kMinBpm, kMaxBpm);

  int skipVal = params.getInt("skip_val", skipInterval_ - 1);
  if (params.contains("skipval")) {
    skipVal = params.getInt("skipval", skipVal);
  }
  skipVal = std::clamp(skipVal, 0, kMaxSkipInterval - 1);
  const int skipInterval = std::clamp(skipVal + 1, kMinSkipInterval, kMaxSkipInterval);

  int skipFirst = params.getInt("skip_first", skipFirstCount_);
  if (params.contains("skipfirst")) {
    skipFirst = params.getInt("skipfirst", skipFirst);
  }
  skipFirst = std::clamp(skipFirst, 0, kMaxSkipFirst);

  avs::effects::GateOptions options = gateOptions_;
  options.enableOnBeat = params.getBool("gate_enable", options.enableOnBeat);
  options.stickyToggle = params.getBool("gate_sticky", options.stickyToggle);
  options.onlySticky = params.getBool("gate_only_sticky", options.onlySticky);
  options.holdFrames = std::max(0, params.getInt("gate_hold", options.holdFrames));

  const int renderRegister = clampRegisterIndex(params.getInt("gate_register", gateRenderRegister_ >= 0 ? gateRenderRegister_ + 1 : -1));
  const int flagRegister = clampRegisterIndex(params.getInt("gate_flag_register", gateFlagRegister_ >= 0 ? gateFlagRegister_ + 1 : -1));

  const bool parametersChanged = enabled_ != enabled || mode_ != newMode || bpm_ != bpm || skipInterval_ != skipInterval ||
                                 skipFirstCount_ != skipFirst || gateRenderRegister_ != renderRegister ||
                                 gateFlagRegister_ != flagRegister || gateOptions_.enableOnBeat != options.enableOnBeat ||
                                 gateOptions_.stickyToggle != options.stickyToggle ||
                                 gateOptions_.onlySticky != options.onlySticky || gateOptions_.holdFrames != options.holdFrames;

  enabled_ = enabled;
  mode_ = newMode;
  bpm_ = bpm;
  skipInterval_ = skipInterval;
  skipFirstCount_ = skipFirst;
  gateRenderRegister_ = renderRegister;
  gateFlagRegister_ = flagRegister;
  gateOptions_ = options;

  if (parametersChanged) {
    configureGate();
    resetState();
  }
}

bool CustomBpmEffect::render(avs::core::RenderContext& context) {
  if (!enabled_) {
    return true;
  }

  const avs::audio::Analysis* analysis = context.audioAnalysis;
  bool baseBeat = context.audioBeat;
  if (analysis) {
    baseBeat = analysis->beat;
  }

  bool overrideBeat = false;
  bool eventPulse = baseBeat;

  if (mode_ == Mode::Arbitrary) {
    overrideBeat = true;
    const double interval = intervalSeconds();
    bool emit = false;
    if (interval > 0.0) {
      accumulatorSeconds_ += std::max(0.0, context.deltaSeconds);
      if (accumulatorSeconds_ >= interval) {
        emit = true;
        accumulatorSeconds_ = std::fmod(accumulatorSeconds_, interval);
      }
    }
    eventPulse = emit;
  } else {
    if (baseBeat) {
      ++beatsSeen_;
    }
    if (skipFirstCount_ > 0 && baseBeat && beatsSeen_ <= skipFirstCount_) {
      overrideBeat = true;
      eventPulse = false;
    } else if (mode_ == Mode::Skip) {
      overrideBeat = true;
      if (baseBeat) {
        ++skipCounter_;
        if (skipCounter_ >= skipInterval_) {
          skipCounter_ = 0;
          eventPulse = true;
        } else {
          eventPulse = false;
        }
      } else {
        eventPulse = false;
      }
    } else if (mode_ == Mode::Invert) {
      overrideBeat = true;
      eventPulse = !baseBeat;
    } else {
      eventPulse = baseBeat;
    }
  }

  const avs::effects::GateResult gate = gate_.step(eventPulse);
  writeGateRegisters(context, gate);

  if (overrideBeat) {
    context.audioBeat = gate.render;
  } else {
    // When passing through, keep the gate in sync with the upstream beat but do not alter it.
    context.audioBeat = (gate.flag == avs::effects::GateFlag::Beat) ? true : baseBeat;
  }

  return true;
}

}  // namespace avs::effects::misc
