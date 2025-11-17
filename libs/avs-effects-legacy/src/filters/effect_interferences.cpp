#include <avs/effects/filters/effect_interferences.h>

#include <algorithm>
#include <cmath>
#include <random>
#include <string>
#include <string_view>

#include <avs/effects/filters/filter_common.h>

namespace avs::effects::filters {

namespace {
constexpr int kMaxAmplitude = 255;
constexpr int kMaxNoise = 255;
constexpr float kTwoPi = 6.28318530717958647692f;

Interferences::Mode parseMode(std::string_view value, Interferences::Mode fallback) {
  if (value == "add") {
    return Interferences::Mode::Add;
  }
  if (value == "subtract" || value == "sub") {
    return Interferences::Mode::Subtract;
  }
  if (value == "multiply" || value == "mul") {
    return Interferences::Mode::Multiply;
  }
  return fallback;
}

std::array<int, 3> unpackTint(int value) {
  const std::uint32_t v = static_cast<std::uint32_t>(value);
  return {static_cast<int>((v >> 16) & 0xFFu), static_cast<int>((v >> 8) & 0xFFu), static_cast<int>(v & 0xFFu)};
}

}  // namespace

void Interferences::setParams(const avs::core::ParamBlock& params) {
  amplitude_ = std::clamp(params.getInt("amplitude", amplitude_), 0, kMaxAmplitude);
  period_ = std::max(1, params.getInt("period", period_));
  speed_ = params.getInt("speed", speed_);
  noise_ = std::clamp(params.getInt("noise", noise_), 0, kMaxNoise);
  phase_ = params.getInt("phase", phase_);
  vertical_ = params.getBool("vertical", vertical_);
  tint_ = unpackTint(params.getInt("tint", (tint_[0] << 16) | (tint_[1] << 8) | tint_[2]));
  const std::string modeString = params.getString("mode", "");
  if (!modeString.empty()) {
    mode_ = parseMode(modeString, mode_);
  }
}

bool Interferences::render(avs::core::RenderContext& context) {
  if (!hasFramebuffer(context) || amplitude_ <= 0) {
    return true;
  }

  const int width = context.width;
  const int height = context.height;
  std::uint8_t* pixels = context.framebuffer.data;
  const float invPeriod = kTwoPi / static_cast<float>(period_);
  const int phaseShift = phase_ + speed_ * static_cast<int>(context.frameIndex);

  std::mt19937 rng;
  std::uniform_int_distribution<int> noiseDist(-noise_, noise_);
  if (noise_ > 0) {
    std::uint32_t base = context.rng.nextUint32() ^ static_cast<std::uint32_t>(phaseShift * 1315423911);
    if (base == 0) {
      base = 0x6C8E9CF5u;
    }
    rng.seed(base);
  }

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const int coordPrimary = vertical_ ? x : y;
      const int coordSecondary = vertical_ ? y : x;
      const float anglePrimary = (static_cast<float>(coordPrimary) + static_cast<float>(phaseShift)) * invPeriod;
      const float angleSecondary = (static_cast<float>(coordSecondary) + static_cast<float>(phaseShift)) *
                                   (invPeriod * 0.37f);
      float wave = std::sin(anglePrimary) * 0.75f + std::sin(angleSecondary) * 0.25f;
      int base = static_cast<int>(std::lround(wave * static_cast<float>(amplitude_)));
      if (noise_ > 0) {
        base += noiseDist(rng);
      }
      base = std::clamp(base, -255, 255);

      std::uint8_t* px = pixels + (static_cast<std::size_t>(y) * width + static_cast<std::size_t>(x)) * 4u;
      for (int channel = 0; channel < 3; ++channel) {
        const int tinted = (base * tint_[channel]) / 255;
        switch (mode_) {
          case Mode::Add:
            px[channel] = saturatingAdd(px[channel], tinted);
            break;
          case Mode::Subtract:
            px[channel] = saturatingAdd(px[channel], -tinted);
            break;
          case Mode::Multiply: {
            const int factor = std::clamp(255 + tinted, 0, 512);
            const int value = (static_cast<int>(px[channel]) * factor + 127) / 255;
            px[channel] = clampByte(value);
            break;
          }
        }
      }
    }
  }

  return true;
}

}  // namespace avs::effects::filters

