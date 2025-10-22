#pragma once

#include <array>
#include <memory>
#include <string>

#include <avs/core/IEffect.hpp>
#include <avs/runtime/GlobalState.hpp>
#include <avs/runtime/script/eel_runtime.h>

namespace avs::effects {

class Globals : public avs::core::IEffect {
 public:
  Globals();
  ~Globals() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  void ensureRuntime();
  bool compileScripts();
  void syncFromState(const avs::runtime::GlobalState& state);
  void syncToState(avs::runtime::GlobalState& state) const;

  std::unique_ptr<avs::runtime::script::EelRuntime> runtime_;
  std::array<avs::runtime::script::EelVarPointer, avs::runtime::GlobalState::kRegisterCount>
      registerPointers_{};
  EEL_F* frameVar_ = nullptr;
  EEL_F* timeVar_ = nullptr;

  std::string initScript_;
  std::string frameScript_;

  bool dirty_ = true;
  bool compiled_ = false;
  bool initExecuted_ = false;
  double timeSeconds_ = 0.0;
};

}  // namespace avs::effects

