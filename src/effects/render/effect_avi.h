#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include "avs/core/IEffect.hpp"

/**
 * @brief Minimal implementation of the legacy "Render / AVI" effect.
 *
 * The original module streamed frames from AVI containers. The modern runtime
 * does not yet ship with a cross-platform AVI decoder, so this effect renders a
 * placeholder background or an optional thumbnail image extracted from a PNG or
 * JPEG file.
 */
class Effect_RenderAvi : public avs::core::IEffect {
 public:
  Effect_RenderAvi();
  ~Effect_RenderAvi() override;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  struct ImageBuffer {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> pixels;

    [[nodiscard]] bool valid() const noexcept {
      return width > 0 && height > 0 && !pixels.empty();
    }
  };

  void ensureThumbnailLoaded();
  void fillBackground(avs::core::RenderContext& context) const;
  void drawPlaceholder(avs::core::RenderContext& context) const;
  void drawThumbnail(avs::core::RenderContext& context, const ImageBuffer& thumbnail) const;

  bool enabled_ = true;
  std::string sourcePath_;
  std::string fallbackImagePath_;

  bool attemptedLoad_ = false;
  ImageBuffer thumbnail_;
  mutable std::mutex loadMutex_;
};
