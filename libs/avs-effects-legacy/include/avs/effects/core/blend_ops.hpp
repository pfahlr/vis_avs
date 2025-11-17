#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace avs::effects {

enum class BlendOp {
  Additive,
  Alpha,
  Alpha2,
  AlphaSlide,
  Blend,
  BlendSlide,
  Replace,
  DefaultBlend,
  DefrendBlend,
  Above,
  Below,
};

struct BlendConfig {
  std::uint8_t alpha = 255;
  std::uint8_t alpha2 = 255;
  std::uint8_t slide = 255;
};

std::optional<BlendOp> parseBlendOpToken(std::string_view token);
BlendOp parseBlendOpOrDefault(std::string_view token, BlendOp fallback);

void blendPixelInPlace(BlendOp op, const BlendConfig& config, std::uint8_t* dst,
                       const std::uint8_t* src);

std::array<std::uint8_t, 4> blendPixel(BlendOp op, const BlendConfig& config,
                                       const std::array<std::uint8_t, 4>& dst,
                                       const std::array<std::uint8_t, 4>& src);

}  // namespace avs::effects
