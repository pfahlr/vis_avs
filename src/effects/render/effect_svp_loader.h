#pragma once

#include <memory>

#include "avs/core/IEffect.hpp"

namespace avs::effects::render {

/**
 * @brief Loads legacy SVP/UVS visualization plugins when supported.
 */
class SvpLoader : public avs::core::IEffect {
 public:
  SvpLoader();
  ~SvpLoader() override;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace avs::effects::render

