#include "script/eel_runtime.h"

#include <algorithm>
#include <mutex>

namespace {
std::once_flag gEelInitFlag;
}  // namespace

namespace avs::runtime::script {

void EelRuntime::ensureGlobalInit() { std::call_once(gEelInitFlag, []() { NSEEL_init(); }); }

EelRuntime::EelRuntime() {
  ensureGlobalInit();
  ctx_ = NSEEL_VM_alloc();
  rng_.seed(0);
  NSEEL_VM_SetCustomFuncThis(ctx_, this);
  NSEEL_addfunc_retval("rand", 0, NSEEL_PProc_THIS, (void*)funcRand);
  NSEEL_addfunc_retval("clamp", 3, NSEEL_PProc_THIS, (void*)funcClamp);
  NSEEL_addfunc_retval("smooth", 3, NSEEL_PProc_THIS, (void*)funcSmooth);

  for (std::size_t i = 0; i < qRegisters_.size(); ++i) {
    const std::string name = "q" + std::to_string(i + 1);
    qRegisters_[i] = NSEEL_VM_regvar(ctx_, name.c_str());
    if (qRegisters_[i]) {
      *qRegisters_[i] = 0.0;
    }
  }
}

EelRuntime::~EelRuntime() {
  clearAll();
  if (ctx_) {
    NSEEL_VM_free(ctx_);
    ctx_ = nullptr;
  }
}

EEL_F* EelRuntime::registerVar(std::string_view name) {
  const std::string owned(name);
  EEL_F* var = NSEEL_VM_regvar(ctx_, owned.c_str());
  if (var) {
    *var = 0.0;
  }
  return var;
}

bool EelRuntime::compile(Stage stage, std::string_view code, std::string& errorMessage) {
  clear(stage);
  if (code.empty()) {
    return true;
  }
  const std::string owned(code);
  NSEEL_CODEHANDLE handle = NSEEL_code_compile(ctx_, owned.c_str(), 0);
  if (!handle) {
    if (char* err = NSEEL_code_getcodeerror(ctx_)) {
      errorMessage = err;
    } else {
      errorMessage = "unknown compile error";
    }
    return false;
  }
  handles_[stageIndex(stage)] = handle;
  return true;
}

void EelRuntime::clear(Stage stage) {
  const int idx = stageIndex(stage);
  if (handles_[idx]) {
    NSEEL_code_free(handles_[idx]);
    handles_[idx] = nullptr;
  }
}

void EelRuntime::clearAll() {
  clear(Stage::kInit);
  clear(Stage::kFrame);
  clear(Stage::kPixel);
}

ExecuteResult EelRuntime::execute(Stage stage, ExecutionBudget* budget) {
  ExecuteResult result;
  const int idx = stageIndex(stage);
  NSEEL_CODEHANDLE handle = handles_[idx];
  if (!handle) {
    return result;
  }
  if (budget && budget->maxInstructionBytes > 0) {
    int cost = 0;
    if (int* stats = NSEEL_code_getstats(handle)) {
      cost = stats[1] + stats[2];
    }
    if (budget->usedInstructionBytes + cost > budget->maxInstructionBytes) {
      result.success = false;
      result.message = "instruction budget exceeded";
      return result;
    }
    budget->usedInstructionBytes += cost;
  }
  NSEEL_code_execute(handle);
  return result;
}

void EelRuntime::setRandomSeed(std::uint32_t seed) { rng_.seed(seed); }

std::array<double, 32> EelRuntime::snapshotQ() const {
  std::array<double, 32> values{};
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (qRegisters_[i]) {
      values[i] = static_cast<double>(*qRegisters_[i]);
    } else {
      values[i] = 0.0;
    }
  }
  return values;
}

std::array<EelVarPointer, 32> EelRuntime::qPointers() const {
  return qRegisters_;
}

EEL_F EelRuntime::funcRand(void* opaque) {
  auto* self = static_cast<EelRuntime*>(opaque);
  const double value = static_cast<double>(self->rng_()) / static_cast<double>(self->rng_.max());
  return static_cast<EEL_F>(value);
}

EEL_F EelRuntime::funcClamp(void* /*opaque*/, EEL_F* x, EEL_F* lo, EEL_F* hi) {
  const EEL_F value = *x;
  return std::clamp(value, *lo, *hi);
}

EEL_F EelRuntime::funcSmooth(void* /*opaque*/, EEL_F* prev, EEL_F* value, EEL_F* a) {
  return *prev + (*value - *prev) * (*a);
}

}  // namespace avs::runtime::script

