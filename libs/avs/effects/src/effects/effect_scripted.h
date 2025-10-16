#pragma once

#include <array>
#include <memory>
#include <string>
#include <string_view>

#include "avs/core/IEffect.hpp"
#include "avs/runtime/GlobalState.hpp"
#include "avs/runtime/script/eel_runtime.h"

namespace avs::effects {

class ScriptedEffect : public avs::core::IEffect {
 public:
  ScriptedEffect();
  ~ScriptedEffect() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  struct OverlayStyle;

  void ensureRuntime();
  bool compileScripts();
  void rebuildScriptsFromParams(const avs::core::ParamBlock& params);
  bool executeStage(avs::runtime::script::EelRuntime::Stage stage,
                    avs::runtime::script::ExecutionBudget& budget,
                    std::string_view label);
  void updateBindings(const avs::core::RenderContext& context);
  void applyPixelScript(avs::core::RenderContext& context,
                        avs::runtime::script::ExecutionBudget& budget);
  void drawOverlays(avs::core::RenderContext& context) const;
  void drawRegisterOverlay(avs::core::RenderContext& context, int originY) const;
  void drawErrorOverlay(avs::core::RenderContext& context, int originY, std::string_view message) const;
  void drawText(avs::core::RenderContext& context,
               int originX,
               int originY,
               std::string_view text,
               const OverlayStyle& style) const;
  void loadGlobalRegisters(const avs::core::RenderContext& context);
  void storeGlobalRegisters(avs::core::RenderContext& context) const;

  std::unique_ptr<avs::runtime::script::EelRuntime> runtime_;

  EEL_F *time_ = nullptr, *frame_ = nullptr;
  EEL_F *widthVar_ = nullptr, *heightVar_ = nullptr;
  EEL_F *xVar_ = nullptr, *yVar_ = nullptr;
  EEL_F *redVar_ = nullptr, *greenVar_ = nullptr, *blueVar_ = nullptr;
  EEL_F *bassVar_ = nullptr, *midVar_ = nullptr, *trebVar_ = nullptr;
  EEL_F* arbValVar_ = nullptr;
  std::array<avs::runtime::script::EelVarPointer, avs::runtime::GlobalState::kRegisterCount>
      globalVars_{};

  std::string libraryScript_;
  std::string initScript_;
  std::string frameScript_;
  std::string pixelScript_;

  bool dirty_ = true;
  bool initExecuted_ = false;
  double timeSeconds_ = 0.0;
  float arbValParam_ = 0.0f;

  std::string compileErrorStage_;
  std::string compileErrorDetail_;
  std::string runtimeErrorStage_;
  std::string runtimeErrorDetail_;
};

}  // namespace avs::effects

