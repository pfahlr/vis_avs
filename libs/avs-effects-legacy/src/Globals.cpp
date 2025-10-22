#include <avs/effects/Globals.hpp>

namespace avs::effects {

namespace {
constexpr int kInstructionBudgetBytes = 200000;
}

Globals::Globals() = default;

void Globals::ensureRuntime() {
  if (runtime_) {
    return;
  }
  runtime_ = std::make_unique<avs::runtime::script::EelRuntime>();
  frameVar_ = runtime_->registerVar("frame");
  timeVar_ = runtime_->registerVar("time");
  for (std::size_t i = 0; i < registerPointers_.size(); ++i) {
    const std::string name = "g" + std::to_string(i + 1);
    registerPointers_[i] = runtime_->registerVar(name);
  }
}

bool Globals::compileScripts() {
  if (!runtime_) {
    return false;
  }
  std::string error;
  if (!runtime_->compile(avs::runtime::script::EelRuntime::Stage::kInit, initScript_, error)) {
    return false;
  }
  if (!runtime_->compile(avs::runtime::script::EelRuntime::Stage::kFrame, frameScript_, error)) {
    return false;
  }
  return true;
}

void Globals::syncFromState(const avs::runtime::GlobalState& state) {
  for (std::size_t i = 0; i < registerPointers_.size(); ++i) {
    if (registerPointers_[i]) {
      *registerPointers_[i] = static_cast<EEL_F>(state.registers[i]);
    }
  }
}

void Globals::syncToState(avs::runtime::GlobalState& state) const {
  for (std::size_t i = 0; i < registerPointers_.size(); ++i) {
    if (registerPointers_[i]) {
      state.registers[i] = static_cast<double>(*registerPointers_[i]);
    }
  }
}

bool Globals::render(avs::core::RenderContext& context) {
  if (!context.globals) {
    return true;
  }
  ensureRuntime();

  if (dirty_) {
    compiled_ = compileScripts();
    initExecuted_ = false;
    dirty_ = false;
  }
  if (!compiled_) {
    return false;
  }

  runtime_->setRandomSeed(context.rng.nextUint32());
  syncFromState(*context.globals);

  if (frameVar_) {
    *frameVar_ = static_cast<EEL_F>(context.frameIndex);
  }
  timeSeconds_ += context.deltaSeconds;
  if (timeVar_) {
    *timeVar_ = static_cast<EEL_F>(timeSeconds_);
  }

  avs::runtime::script::ExecutionBudget budget;
  budget.maxInstructionBytes = kInstructionBudgetBytes;

  if (!initExecuted_) {
    auto result = runtime_->execute(avs::runtime::script::EelRuntime::Stage::kInit, &budget);
    if (!result.success) {
      compiled_ = false;
      return false;
    }
    initExecuted_ = true;
  }

  auto result = runtime_->execute(avs::runtime::script::EelRuntime::Stage::kFrame, &budget);
  if (!result.success) {
    compiled_ = false;
    return false;
  }

  syncToState(*context.globals);
  return true;
}

void Globals::setParams(const avs::core::ParamBlock& params) {
  auto select = [&](const std::string& key, const std::string& fallback) {
    if (params.contains(key)) {
      return params.getString(key, fallback);
    }
    return fallback;
  };

  const std::string newInit = select("init", initScript_);
  const std::string newFrame = select("frame", frameScript_);

  if (newInit != initScript_ || newFrame != frameScript_) {
    initScript_ = newInit;
    frameScript_ = newFrame;
    dirty_ = true;
  }
}

}  // namespace avs::effects

