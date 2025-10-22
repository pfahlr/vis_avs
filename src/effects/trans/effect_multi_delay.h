#pragma once

#include <string>

#include "avs/core/IEffect.hpp"

namespace avs::effects::trans {

/**
 * @brief Implements the legacy Holden05 Multi Delay transformation.
 *
 * The effect exposes six shared delay buffers. Any instance can store the
 * current frame into a buffer or fetch a previously stored frame from it.
 * Buffers can operate on fixed frame delays or reuse the last measured
 * beat duration to determine their history length.
 */
class MultiDelay : public avs::core::IEffect {
 public:
  MultiDelay();
  ~MultiDelay() override;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  enum class Mode { Passthrough = 0, Store = 1, Fetch = 2 };

  void setMode(int value);
  bool setModeFromString(const std::string& value);
  void setActiveBuffer(int index);
  bool setActiveBufferFromString(const std::string& value);

  Mode mode_{Mode::Passthrough};
  int activeBuffer_{0};
};

}  // namespace avs::effects::trans
