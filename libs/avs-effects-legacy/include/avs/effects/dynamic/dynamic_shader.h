#pragma once

#include <array>
#include <memory>
#include <string>

#include <avs/runtime/script/eel_runtime.h>
#include <avs/effects/dynamic/frame_warp.h>

namespace avs::effects {

// Shared implementation for the Dynamic Movement style effects. Derived classes
// only need to translate the EEL state after executing the pixel script into a
// normalized sample coordinate.
class DynamicShaderEffect : public FrameWarpEffect {
 public:
  DynamicShaderEffect();
  ~DynamicShaderEffect() override = default;

  void setParams(const avs::core::ParamBlock& params) override;
  bool render(avs::core::RenderContext& context) override;

 protected:
  struct SampleCoord {
    float x{0.0f};
    float y{0.0f};
  };

  virtual SampleCoord resolveSample() const = 0;

  EEL_F* xVar_{nullptr};
  EEL_F* yVar_{nullptr};
  EEL_F* origXVar_{nullptr};
  EEL_F* origYVar_{nullptr};
  EEL_F* radiusVar_{nullptr};
  EEL_F* angleVar_{nullptr};
  EEL_F* dxVar_{nullptr};
  EEL_F* dyVar_{nullptr};

  void setWrapEnabled(bool enabled) { wrap_ = enabled; }

 private:
  void ensureRuntime();
  bool compileScripts();
  bool executeStage(avs::runtime::script::EelRuntime::Stage stage);
  void bindFrame(const avs::core::RenderContext& context);
  void bindPixel(int px, int py, const avs::core::RenderContext& context);

  std::unique_ptr<avs::runtime::script::EelRuntime> runtime_;
  avs::runtime::script::ExecutionBudget budget_{};

  EEL_F* frameVar_{nullptr};
  EEL_F* timeVar_{nullptr};
  EEL_F* bassVar_{nullptr};
  EEL_F* midVar_{nullptr};
  EEL_F* trebVar_{nullptr};
  EEL_F* widthVar_{nullptr};
  EEL_F* heightVar_{nullptr};

  std::string initScript_;
  std::string frameScript_;
  std::string pixelScript_;

  bool dirty_{true};
  bool initExecuted_{false};
  double timeSeconds_{0.0};
  bool wrap_{false};
};

}  // namespace avs::effects

