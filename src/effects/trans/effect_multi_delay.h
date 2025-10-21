#pragma once

#include "avs/core/IEffect.hpp"

namespace avs::effects::trans {

class MultiDelay : public avs::core::IEffect {
 public:
  MultiDelay();
  ~MultiDelay() override;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  enum class Mode { Passthrough = 0, Store = 1, Fetch = 2 };

  void setMode(int value);
  void setActiveBuffer(int index);

  Mode mode_{Mode::Passthrough};
  int activeBuffer_{0};
};

}  // namespace avs::effects::trans
