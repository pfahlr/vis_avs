#pragma once

#include <array>
#include <cstdint>
#include <random>
#include <string>
#include <string_view>

#include "ns-eel-addfuncs.h"
#include "ns-eel.h"

namespace avs::runtime::script {

using EelVarPointer = double*;

struct ExecutionBudget {
  int maxInstructionBytes = 0;
  int usedInstructionBytes = 0;
};

struct ExecuteResult {
  bool success = true;
  std::string message;
};

class EelRuntime {
 public:
  enum class Stage { kInit = 0, kFrame = 1, kPixel = 2 };

  EelRuntime();
  ~EelRuntime();

  EelRuntime(const EelRuntime&) = delete;
  EelRuntime& operator=(const EelRuntime&) = delete;
  EelRuntime(EelRuntime&&) = delete;
  EelRuntime& operator=(EelRuntime&&) = delete;

  EEL_F* registerVar(std::string_view name);

  [[nodiscard]] bool compile(Stage stage, std::string_view code, std::string& errorMessage);
  void clear(Stage stage);
  void clearAll();

  ExecuteResult execute(Stage stage, ExecutionBudget* budget);

  void setRandomSeed(std::uint32_t seed);

  [[nodiscard]] std::array<double, 32> snapshotQ() const;
  [[nodiscard]] std::array<EelVarPointer, 32> qPointers() const;

 private:
  static void ensureGlobalInit();

  static EEL_F NSEEL_CGEN_CALL funcRand(void* opaque);
  static EEL_F NSEEL_CGEN_CALL funcClamp(void* opaque, EEL_F* x, EEL_F* lo, EEL_F* hi);
  static EEL_F NSEEL_CGEN_CALL funcSmooth(void* opaque, EEL_F* prev, EEL_F* value, EEL_F* a);

  static int stageIndex(Stage stage) { return static_cast<int>(stage); }

  NSEEL_VMCTX ctx_ = nullptr;
  NSEEL_CODEHANDLE handles_[3]{};
  std::mt19937 rng_{};
  std::array<EelVarPointer, 32> qRegisters_{};
};

}  // namespace avs::runtime::script

