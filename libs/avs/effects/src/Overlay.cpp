#include "avs/effects/Overlay.hpp"

#include <algorithm>

namespace {
std::array<std::uint8_t, 4> colorFromInt(int value, std::uint8_t alphaDefault) {
  const std::uint32_t v = static_cast<std::uint32_t>(value);
  return {static_cast<std::uint8_t>((v >> 16) & 0xFFu), static_cast<std::uint8_t>((v >> 8) & 0xFFu),
          static_cast<std::uint8_t>(v & 0xFFu), alphaDefault};
}

std::uint8_t clampByte(int value) {
  return static_cast<std::uint8_t>(value < 0 ? 0 : (value > 255 ? 255 : value));
}
}  // namespace

namespace avs::effects {

bool Overlay::render(avs::core::RenderContext& context) {
  if (!context.framebuffer.data || context.framebuffer.size == 0) {
    return true;
  }
  BlendConfig config;
  config.alpha = alpha_;
  config.alpha2 = alpha2_;
  config.slide = slide_;
  const auto blended = blendPixel(op_, config, background_, foreground_);
  std::uint8_t* data = context.framebuffer.data;
  const std::size_t size = context.framebuffer.size;
  for (std::size_t offset = 0; offset + 3 < size; offset += 4) {
    std::copy(blended.begin(), blended.end(), data + offset);
  }
  return true;
}

void Overlay::setParams(const avs::core::ParamBlock& params) {
  const std::string opToken = params.getString("op", params.getString("mode", "replace"));
  op_ = parseBlendOpOrDefault(opToken, BlendOp::Replace);

  const int bgValue = params.getInt("bg", params.getInt("background", 0));
  const int fgValue = params.getInt("fg", params.getInt("foreground", 0));
  const int bgAlpha = params.getInt("bg_alpha", 255);
  const int fgAlpha = params.getInt("fg_alpha", 255);
  background_ = colorFromInt(bgValue, clampByte(bgAlpha));
  foreground_ = colorFromInt(fgValue, clampByte(fgAlpha));

  alpha_ = clampByte(params.getInt("alpha", 255));
  alpha2_ = clampByte(params.getInt("alpha2", params.getInt("alpha_2", alpha_)));
  slide_ = clampByte(params.getInt("slide", params.getInt("blend", alpha_)));
}

}  // namespace avs::effects
