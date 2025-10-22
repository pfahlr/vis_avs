#include "effects/dynamic/dynamic_shader.h"

#include <cmath>
#include <iostream>

#include <avs/audio/analyzer.h>

namespace avs::effects {

namespace {
constexpr int kInstructionBudget = 4000000;
constexpr double kPi = 3.1415926535897932384626433832795;
}

DynamicShaderEffect::DynamicShaderEffect() { budget_.maxInstructionBytes = kInstructionBudget; }

void DynamicShaderEffect::setParams(const avs::core::ParamBlock& params) {
  auto selectString = [&](const std::string& key, std::string& target) {
    if (params.contains(key)) {
      target = params.getString(key, target);
      dirty_ = true;
    }
  };

  selectString("init", initScript_);
  selectString("frame", frameScript_);
  selectString("pixel", pixelScript_);
  if (params.contains("wrap")) {
    wrap_ = params.getBool("wrap", wrap_);
  }
}

bool DynamicShaderEffect::render(avs::core::RenderContext& context) {
  if (!prepareHistory(context)) {
    return true;
  }

  ensureRuntime();
  if (!runtime_) {
    return true;
  }

  if (dirty_ && !compileScripts()) {
    return false;
  }

  if (!initExecuted_) {
    if (!executeStage(avs::runtime::script::EelRuntime::Stage::kInit)) {
      return false;
    }
    initExecuted_ = true;
  }

  bindFrame(context);

  if (!executeStage(avs::runtime::script::EelRuntime::Stage::kFrame)) {
    return false;
  }

  const int width = historyWidth();
  const int height = historyHeight();
  if (width <= 0 || height <= 0) {
    return true;
  }

  for (int py = 0; py < height; ++py) {
    for (int px = 0; px < width; ++px) {
      bindPixel(px, py, context);
      if (!executeStage(avs::runtime::script::EelRuntime::Stage::kPixel)) {
        return false;
      }
      const SampleCoord coord = resolveSample();
      const auto color = sampleHistory(coord.x, coord.y, wrap_);
      const std::size_t index = (static_cast<std::size_t>(py) * static_cast<std::size_t>(width) +
                                 static_cast<std::size_t>(px)) * 4u;
      context.framebuffer.data[index + 0] = color[0];
      context.framebuffer.data[index + 1] = color[1];
      context.framebuffer.data[index + 2] = color[2];
      context.framebuffer.data[index + 3] = color[3];
    }
  }

  storeHistory(context);
  return true;
}

void DynamicShaderEffect::ensureRuntime() {
  if (runtime_) {
    return;
  }
  runtime_ = std::make_unique<avs::runtime::script::EelRuntime>();
  runtime_->setRandomSeed(0);
  xVar_ = runtime_->registerVar("x");
  yVar_ = runtime_->registerVar("y");
  origXVar_ = runtime_->registerVar("orig_x");
  origYVar_ = runtime_->registerVar("orig_y");
  radiusVar_ = runtime_->registerVar("d");
  angleVar_ = runtime_->registerVar("angle");
  dxVar_ = runtime_->registerVar("dx");
  dyVar_ = runtime_->registerVar("dy");
  frameVar_ = runtime_->registerVar("frame");
  timeVar_ = runtime_->registerVar("time");
  bassVar_ = runtime_->registerVar("bass");
  midVar_ = runtime_->registerVar("mid");
  trebVar_ = runtime_->registerVar("treb");
  widthVar_ = runtime_->registerVar("width");
  heightVar_ = runtime_->registerVar("height");
  dirty_ = true;
}

bool DynamicShaderEffect::compileScripts() {
  if (!runtime_) {
    return false;
  }
  std::string error;
  if (!runtime_->compile(avs::runtime::script::EelRuntime::Stage::kInit, initScript_, error)) {
    std::clog << "dyn shader init compile failed: " << error << '\n';
    return false;
  }
  if (!runtime_->compile(avs::runtime::script::EelRuntime::Stage::kFrame, frameScript_, error)) {
    std::clog << "dyn shader frame compile failed: " << error << '\n';
    return false;
  }
  if (!runtime_->compile(avs::runtime::script::EelRuntime::Stage::kPixel, pixelScript_, error)) {
    std::clog << "dyn shader pixel compile failed: " << error << '\n';
    return false;
  }
  dirty_ = false;
  return true;
}

bool DynamicShaderEffect::executeStage(avs::runtime::script::EelRuntime::Stage stage) {
  if (!runtime_) {
    return false;
  }
  avs::runtime::script::ExecuteResult result = runtime_->execute(stage, &budget_);
  if (!result.success) {
    std::clog << "dyn shader runtime error: " << result.message << '\n';
    return false;
  }
  return true;
}

void DynamicShaderEffect::bindFrame(const avs::core::RenderContext& context) {
  if (!runtime_) {
    return;
  }
  budget_.usedInstructionBytes = 0;
  if (frameVar_) {
    *frameVar_ = static_cast<EEL_F>(context.frameIndex);
  }
  timeSeconds_ += context.deltaSeconds;
  if (timeVar_) {
    *timeVar_ = static_cast<EEL_F>(timeSeconds_);
  }
  const avs::audio::Analysis* analysis = context.audioAnalysis;
  if (analysis) {
    if (bassVar_) *bassVar_ = analysis->bass;
    if (midVar_) *midVar_ = analysis->mid;
    if (trebVar_) *trebVar_ = analysis->treb;
  } else {
    if (bassVar_) *bassVar_ = 0.0;
    if (midVar_) *midVar_ = 0.0;
    if (trebVar_) *trebVar_ = 0.0;
  }
  if (widthVar_) {
    *widthVar_ = static_cast<EEL_F>(historyWidth());
  }
  if (heightVar_) {
    *heightVar_ = static_cast<EEL_F>(historyHeight());
  }
}

void DynamicShaderEffect::bindPixel(int px, int py, const avs::core::RenderContext& context) {
  const int width = historyWidth();
  const int height = historyHeight();
  if (width <= 0 || height <= 0) {
    return;
  }
  const float normX = (static_cast<float>(px) + 0.5f) / static_cast<float>(width);
  const float normY = (static_cast<float>(py) + 0.5f) / static_cast<float>(height);
  const float x = normX * 2.0f - 1.0f;
  const float y = 1.0f - normY * 2.0f;

  if (origXVar_) *origXVar_ = x;
  if (origYVar_) *origYVar_ = y;
  if (xVar_) *xVar_ = x;
  if (yVar_) *yVar_ = y;

  const float r = std::sqrt(x * x + y * y);
  if (radiusVar_) *radiusVar_ = r;
  float angle = std::atan2(y, x);
  // Convert to AVS' 0..2pi representation for consistency with legacy scripts.
  if (angle < 0.0f) {
    angle += static_cast<float>(2.0 * kPi);
  }
  if (angleVar_) *angleVar_ = angle;
  if (dxVar_) *dxVar_ = 0.0;
  if (dyVar_) *dyVar_ = 0.0;

  (void)context;
}

}  // namespace avs::effects

