#include "effects/trans/effect_multiplier.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace avs::effects::trans {

namespace {
constexpr std::size_t kBytesPerPixel = 4u;
}

Multiplier::Mode Multiplier::decodeMode(int value) noexcept {
  if (value < 0) {
    value = 0;
  } else if (value > 7) {
    value = 7;
  }
  return static_cast<Mode>(value);
}

bool Multiplier::hasFramebuffer(const avs::core::RenderContext& context) noexcept {
  return context.framebuffer.data != nullptr && context.framebuffer.size >= kBytesPerPixel &&
         context.width > 0 && context.height > 0;
}

std::uint8_t Multiplier::multiplyChannel(std::uint8_t value, int factor) noexcept {
  const int result = static_cast<int>(value) * factor;
  return static_cast<std::uint8_t>(result >= 255 ? 255 : result);
}

std::uint8_t Multiplier::scaleChannel(std::uint8_t value, float factor) noexcept {
  const float scaled = static_cast<float>(value) * factor;
  const float clamped = std::clamp(scaled, 0.0f, 255.0f);
  return static_cast<std::uint8_t>(std::lround(clamped));
}

void Multiplier::setParams(const avs::core::ParamBlock& params) {
  bool modeSpecified = false;
  if (params.contains("mode")) {
    mode_ = decodeMode(params.getInt("mode", static_cast<int>(mode_)));
    modeSpecified = true;
  }

  bool customSpecified = false;
  bool uniformSpecified = false;
  const bool wasUsingCustom = useCustomFactors_;
  std::array<bool, 3> channelSpecified{false, false, false};
  if (params.contains("factor")) {
    const float value = params.getFloat("factor", customFactors_[0]);
    customFactors_.fill(value);
    customSpecified = true;
    uniformSpecified = true;
  }
  if (params.contains("factor_r")) {
    customFactors_[0] = params.getFloat("factor_r", customFactors_[0]);
    customSpecified = true;
    channelSpecified[0] = true;
  }
  if (params.contains("factor_g")) {
    customFactors_[1] = params.getFloat("factor_g", customFactors_[1]);
    customSpecified = true;
    channelSpecified[1] = true;
  }
  if (params.contains("factor_b")) {
    customFactors_[2] = params.getFloat("factor_b", customFactors_[2]);
    customSpecified = true;
    channelSpecified[2] = true;
  }

  if (customSpecified) {
    if (!uniformSpecified && !wasUsingCustom) {
      for (std::size_t index = 0; index < channelSpecified.size(); ++index) {
        if (!channelSpecified[index]) {
          customFactors_[index] = 1.0f;
        }
      }
    }
    useCustomFactors_ = true;
  } else if (modeSpecified) {
    useCustomFactors_ = false;
  }
}

bool Multiplier::render(avs::core::RenderContext& context) {
  if (!hasFramebuffer(context)) {
    return true;
  }

  std::uint8_t* const pixels = context.framebuffer.data;
  const std::size_t totalPixels =
      static_cast<std::size_t>(context.width) * static_cast<std::size_t>(context.height);

  if (useCustomFactors_) {
    const float factorR = customFactors_[0];
    const float factorG = customFactors_[1];
    const float factorB = customFactors_[2];
    for (std::size_t index = 0; index < totalPixels; ++index) {
      std::uint8_t* const px = pixels + index * kBytesPerPixel;
      px[0] = scaleChannel(px[0], factorR);
      px[1] = scaleChannel(px[1], factorG);
      px[2] = scaleChannel(px[2], factorB);
    }
    return true;
  }

  switch (mode_) {
    case Mode::kInfinity: {
      for (std::size_t index = 0; index < totalPixels; ++index) {
        std::uint8_t* const px = pixels + index * kBytesPerPixel;
        const bool any = (px[0] | px[1] | px[2]) != 0;
        const std::uint8_t value =
            any ? static_cast<std::uint8_t>(255) : static_cast<std::uint8_t>(0);
        px[0] = value;
        px[1] = value;
        px[2] = value;
      }
      break;
    }
    case Mode::kZero: {
      for (std::size_t index = 0; index < totalPixels; ++index) {
        std::uint8_t* const px = pixels + index * kBytesPerPixel;
        if (px[0] == 255u && px[1] == 255u && px[2] == 255u) {
          continue;
        }
        px[0] = 0u;
        px[1] = 0u;
        px[2] = 0u;
      }
      break;
    }
    case Mode::kX8:
    case Mode::kX4:
    case Mode::kX2: {
      const int factor = mode_ == Mode::kX8 ? 8 : (mode_ == Mode::kX4 ? 4 : 2);
      for (std::size_t index = 0; index < totalPixels; ++index) {
        std::uint8_t* const px = pixels + index * kBytesPerPixel;
        px[0] = multiplyChannel(px[0], factor);
        px[1] = multiplyChannel(px[1], factor);
        px[2] = multiplyChannel(px[2], factor);
      }
      break;
    }
    case Mode::kHalf:
    case Mode::kQuarter:
    case Mode::kEighth: {
      const int shift = mode_ == Mode::kHalf ? 1 : (mode_ == Mode::kQuarter ? 2 : 3);
      for (std::size_t index = 0; index < totalPixels; ++index) {
        std::uint8_t* const px = pixels + index * kBytesPerPixel;
        px[0] = static_cast<std::uint8_t>(px[0] >> shift);
        px[1] = static_cast<std::uint8_t>(px[1] >> shift);
        px[2] = static_cast<std::uint8_t>(px[2] >> shift);
      }
      break;
    }
  }

  return true;
}

}  // namespace avs::effects::trans
