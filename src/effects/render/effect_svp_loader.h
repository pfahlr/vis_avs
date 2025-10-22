#pragma once

#include <memory>

#include "avs/core/IEffect.hpp"
#include "avs/core/ParamBlock.hpp"

namespace avs::effects::render {

/**
 * @brief Loader for legacy Sonique Visualization Plug-ins (SVP).
 *
 * The loader attempts to dynamically load a Windows SVP module at runtime and
 * invoke its rendering entry points. When the current platform does not
 * support SVP modules the effect transparently degrades to a no-op so presets
 * remain functional across platforms.
 */
class SvpLoader : public avs::core::IEffect {
 public:
  SvpLoader();
  ~SvpLoader() override;

  void setParams(const avs::core::ParamBlock& params) override;
  bool render(avs::core::RenderContext& context) override;

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace avs::effects::render
