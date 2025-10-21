#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "avs/core/IEffect.hpp"

class Effect_RenderAvi : public avs::core::IEffect {
 public:
  Effect_RenderAvi();
  ~Effect_RenderAvi() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  struct ImageData {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> pixels;
  };

  [[nodiscard]] bool hasFramebuffer(const avs::core::RenderContext& context) const;
  bool ensureFallbackImageLoaded();
  void drawFallbackImage(avs::core::RenderContext& context) const;
  void drawPlaceholder(avs::core::RenderContext& context) const;

  std::string videoPath_;
  std::string fallbackImagePath_;
  bool fallbackLoadAttempted_ = false;
  bool warnedUnsupported_ = false;
  ImageData fallbackImage_;
};
