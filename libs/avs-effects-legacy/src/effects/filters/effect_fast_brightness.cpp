#include "effects/filters/effect_fast_brightness.h"

#include <algorithm>
#include <cmath>

#include "effects/filters/filter_common.h"

namespace avs::effects::filters {

namespace {
constexpr float kMaxAmount = 8.0f;
constexpr float kMinAmount = 0.0f;
}

void FastBrightness::setParams(const avs::core::ParamBlock& params) {
  if (params.contains("amount")) {
    amount_ = params.getFloat("amount", amount_);
  } else {
    const int mode = params.getInt("mode", 0);
    amount_ = mode <= 0 ? 2.0f : 0.5f;
  }
  amount_ = std::clamp(amount_, kMinAmount, kMaxAmount);
  bias_ = params.getFloat("bias", bias_);
  clampOutput_ = params.getBool("clamp", clampOutput_);
}

bool FastBrightness::render(avs::core::RenderContext& context) {
  if (!hasFramebuffer(context)) {
    return true;
  }

  if (std::abs(amount_ - 1.0f) < 1e-6f && std::abs(bias_) < 1e-3f) {
    return true;
  }

  std::uint8_t* pixels = context.framebuffer.data;
  const std::size_t totalPixels = static_cast<std::size_t>(context.width) * static_cast<std::size_t>(context.height);
  for (std::size_t i = 0; i < totalPixels; ++i) {
    std::uint8_t* px = pixels + i * 4u;
    for (int channel = 0; channel < 3; ++channel) {
      const float scaled = static_cast<float>(px[channel]) * amount_ + bias_;
      const float processed =
          clampOutput_ ? std::clamp(scaled, 0.0f, 255.0f) : scaled;
      const int rounded = static_cast<int>(std::nearbyint(processed));
      const std::uint8_t quantized =
          clampOutput_ ? clampByte(rounded) : static_cast<std::uint8_t>(rounded);
      px[channel] = quantized;
    }
  }
  return true;
}

}  // namespace avs::effects::filters

