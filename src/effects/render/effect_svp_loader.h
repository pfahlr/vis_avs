#pragma once

#include <memory>

#include "avs/core/IEffect.hpp"
#include "avs/core/ParamBlock.hpp"
#include "avs/core/RenderContext.hpp"

namespace avs::effects::render {

/**
 * @brief Loads legacy Sonique Visual Plugin (SVP/UVS) modules when supported.
 *
 * On non-Windows platforms the effect acts as a no-op so that presets stay
 * compatible even when SVP binaries cannot be loaded.
 */
class SvpLoader : public avs::core::IEffect {
 public:
  SvpLoader();
  ~SvpLoader() override;

  void setParams(const avs::core::ParamBlock& params) override;
  bool render(avs::core::RenderContext& context) override;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace avs::effects::render
