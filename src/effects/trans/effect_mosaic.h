#pragma once

#include <cstdint>
#include <vector>

#include "avs/core/IEffect.hpp"

namespace avs::effects::trans {

class Mosaic : public avs::core::IEffect {
 public:
  Mosaic() = default;
  ~Mosaic() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  static bool hasFramebuffer(const avs::core::RenderContext& context) {
    return context.framebuffer.data != nullptr && context.width > 0 && context.height > 0 &&
           context.framebuffer.size >= static_cast<std::size_t>(context.width) *
                                           static_cast<std::size_t>(context.height) * 4u;
  }

  static std::uint32_t blendAdditive(std::uint32_t dst, std::uint32_t src);
  static std::uint32_t blendAverage(std::uint32_t dst, std::uint32_t src);

  void ensureScratch(std::size_t bytes);

  bool enabled_ = true;
  int quality_ = 50;
  int qualityOnBeat_ = 50;
  int durationFrames_ = 15;
  bool blendAdditive_ = false;
  bool blendAverage_ = false;
  bool triggerOnBeat_ = false;

  int remainingBeatFrames_ = 0;
  int currentQuality_ = 50;

  std::vector<std::uint8_t> scratch_;
};

}  // namespace avs::effects::trans
