#include "effects/render/effect_moving_particle.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>

namespace {

std::uint8_t saturatingAdd(std::uint8_t base, std::uint8_t value) {
  const int result = static_cast<int>(base) + static_cast<int>(value);
  return static_cast<std::uint8_t>(result > 255 ? 255 : result);
}

std::uint8_t saturatingSub(std::uint8_t base, std::uint8_t value) {
  const int result = static_cast<int>(base) - static_cast<int>(value);
  return static_cast<std::uint8_t>(result < 0 ? 0 : result);
}

std::uint8_t multiplyByte(std::uint8_t a, std::uint8_t b) {
  const int product = static_cast<int>(a) * static_cast<int>(b);
  return static_cast<std::uint8_t>((product + 127) / 255);
}

std::uint8_t lerpByte(std::uint8_t from, std::uint8_t to, int amount) {
  const int alpha = std::clamp(amount, 0, 255);
  const int invAlpha = 255 - alpha;
  const int blended = static_cast<int>(from) * alpha + static_cast<int>(to) * invAlpha;
  return static_cast<std::uint8_t>((blended + 127) / 255);
}

std::string toLower(std::string_view value) {
  std::string lowered(value);
  std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return lowered;
}

float scaledStep(double deltaSeconds) {
  if (deltaSeconds <= 0.0) {
    return 1.0f;
  }
  const double step = deltaSeconds * 60.0;
  return static_cast<float>(std::clamp(step, 0.0, 8.0));
}

}  // namespace

Effect_RenderMovingParticle::Effect_RenderMovingParticle() : Effect("Render / Moving Particle") {}

void Effect_RenderMovingParticle::setColorFromInt(int rgb) {
  color_[0] = static_cast<std::uint8_t>(rgb & 0xFF);
  color_[1] = static_cast<std::uint8_t>((rgb >> 8) & 0xFF);
  color_[2] = static_cast<std::uint8_t>((rgb >> 16) & 0xFF);
}

void Effect_RenderMovingParticle::setParams(const avs::core::ParamBlock& params) {
  Effect::setParams(params);

  if (params.contains("color")) {
    setColorFromInt(params.getInt("color", 0xFFFFFF));
  } else if (params.contains("colors")) {
    setColorFromInt(params.getInt("colors", 0xFFFFFF));
  }

  const auto readInt = [&params](std::initializer_list<std::string_view> keys, int fallback) {
    for (auto key : keys) {
      std::string keyStr(key);
      if (params.contains(keyStr)) {
        return params.getInt(keyStr, fallback);
      }
    }
    return fallback;
  };

  const int maxDist =
      std::clamp(readInt({"max_distance", "maxdist"}, maxDistance_), 1, kMaxDistance);
  maxDistance_ = maxDist;

  const int baseSize =
      std::clamp(readInt({"size_base", "size"}, sizeBase_), kMinRadius, kMaxRadius);
  const int beatSize =
      std::clamp(readInt({"size_beat", "size2"}, sizeBeat_), kMinRadius, kMaxRadius);
  sizeBase_ = baseSize;
  sizeBeat_ = beatSize;
  sPos_ = std::clamp(sPos_, kMinRadius, kMaxRadius);
  if (params.contains("size_base") || params.contains("size")) {
    sPos_ = sizeBase_;
  }

  const int enabledFlags = params.getInt("enabled", -1);
  if (enabledFlags >= 0) {
    enabled_ = (enabledFlags & 1) != 0;
    beatPulse_ = (enabledFlags & 2) != 0;
  } else {
    enabled_ = params.getBool("enabled", enabled_);
  }

  beatPulse_ = params.getBool("beat_pulse", beatPulse_);

  if (params.contains("blend_mode") || params.contains("blend")) {
    std::string blendValue;
    if (params.contains("blend_mode")) {
      blendValue = params.getString("blend_mode", "");
    }
    if (blendValue.empty() && params.contains("blend")) {
      blendValue = params.getString("blend", "");
    }
    if (!blendValue.empty()) {
      const std::string lowered = toLower(blendValue);
      if (lowered == "replace" || lowered == "copy") {
        blendMode_ = BlendMode::Replace;
      } else if (lowered == "add" || lowered == "additive") {
        blendMode_ = BlendMode::Additive;
      } else if (lowered == "avg" || lowered == "average") {
        blendMode_ = BlendMode::Average;
      } else if (lowered == "line") {
        blendMode_ = BlendMode::Line;
      }
    } else {
      const int blendInt = params.getInt("blend", static_cast<int>(blendMode_));
      switch (blendInt) {
        case 0:
          blendMode_ = BlendMode::Replace;
          break;
        case 1:
          blendMode_ = BlendMode::Additive;
          break;
        case 2:
          blendMode_ = BlendMode::Average;
          break;
        case 3:
          blendMode_ = BlendMode::Line;
          break;
        default:
          break;
      }
    }
  }

  lineBlendMode_ = std::clamp(params.getInt("line_blend_mode", lineBlendMode_), 0, 9);
  lineBlendAdjust_ = std::clamp(
      readInt({"line_blend_amount", "line_blend_adjust", "line_blend_value"}, lineBlendAdjust_), 0,
      255);
}

void Effect_RenderMovingParticle::applyLineBlend(std::uint8_t* pixel) const {
  switch (lineBlendMode_) {
    case 1:  // additive
      pixel[0] = saturatingAdd(pixel[0], color_[0]);
      pixel[1] = saturatingAdd(pixel[1], color_[1]);
      pixel[2] = saturatingAdd(pixel[2], color_[2]);
      return;
    case 2:  // max
      pixel[0] = std::max(pixel[0], color_[0]);
      pixel[1] = std::max(pixel[1], color_[1]);
      pixel[2] = std::max(pixel[2], color_[2]);
      return;
    case 3:  // average
      pixel[0] = static_cast<std::uint8_t>((static_cast<int>(pixel[0]) + color_[0]) / 2);
      pixel[1] = static_cast<std::uint8_t>((static_cast<int>(pixel[1]) + color_[1]) / 2);
      pixel[2] = static_cast<std::uint8_t>((static_cast<int>(pixel[2]) + color_[2]) / 2);
      return;
    case 4:  // subtract
      pixel[0] = saturatingSub(pixel[0], color_[0]);
      pixel[1] = saturatingSub(pixel[1], color_[1]);
      pixel[2] = saturatingSub(pixel[2], color_[2]);
      return;
    case 5:  // reverse subtract
      pixel[0] = saturatingSub(color_[0], pixel[0]);
      pixel[1] = saturatingSub(color_[1], pixel[1]);
      pixel[2] = saturatingSub(color_[2], pixel[2]);
      return;
    case 6:  // multiply
      pixel[0] = multiplyByte(pixel[0], color_[0]);
      pixel[1] = multiplyByte(pixel[1], color_[1]);
      pixel[2] = multiplyByte(pixel[2], color_[2]);
      return;
    case 7:  // adjustable blend
      pixel[0] = lerpByte(color_[0], pixel[0], lineBlendAdjust_);
      pixel[1] = lerpByte(color_[1], pixel[1], lineBlendAdjust_);
      pixel[2] = lerpByte(color_[2], pixel[2], lineBlendAdjust_);
      return;
    case 8:  // xor
      pixel[0] = static_cast<std::uint8_t>(pixel[0] ^ color_[0]);
      pixel[1] = static_cast<std::uint8_t>(pixel[1] ^ color_[1]);
      pixel[2] = static_cast<std::uint8_t>(pixel[2] ^ color_[2]);
      return;
    case 9:  // min
      pixel[0] = std::min(pixel[0], color_[0]);
      pixel[1] = std::min(pixel[1], color_[1]);
      pixel[2] = std::min(pixel[2], color_[2]);
      return;
    default:
      pixel[0] = color_[0];
      pixel[1] = color_[1];
      pixel[2] = color_[2];
      return;
  }
}

void Effect_RenderMovingParticle::applyBlend(std::uint8_t* pixel) const {
  switch (blendMode_) {
    case BlendMode::Replace:
      pixel[0] = color_[0];
      pixel[1] = color_[1];
      pixel[2] = color_[2];
      break;
    case BlendMode::Additive:
      pixel[0] = saturatingAdd(pixel[0], color_[0]);
      pixel[1] = saturatingAdd(pixel[1], color_[1]);
      pixel[2] = saturatingAdd(pixel[2], color_[2]);
      break;
    case BlendMode::Average:
      pixel[0] = static_cast<std::uint8_t>((static_cast<int>(pixel[0]) + color_[0]) / 2);
      pixel[1] = static_cast<std::uint8_t>((static_cast<int>(pixel[1]) + color_[1]) / 2);
      pixel[2] = static_cast<std::uint8_t>((static_cast<int>(pixel[2]) + color_[2]) / 2);
      break;
    case BlendMode::Line:
      applyLineBlend(pixel);
      break;
  }
}

bool Effect_RenderMovingParticle::render(avs::core::RenderContext& context) {
  if (!enabled_) {
    return true;
  }
  if (context.width <= 0 || context.height <= 0) {
    return true;
  }
  if (context.framebuffer.data == nullptr) {
    return true;
  }

  const std::size_t required =
      static_cast<std::size_t>(context.width) * static_cast<std::size_t>(context.height) * 4u;
  if (context.framebuffer.size < required) {
    return true;
  }

  if (context.audioBeat) {
    target_[0] = context.rng.uniform(-16.0f, 16.0f) / 48.0f;
    target_[1] = context.rng.uniform(-16.0f, 16.0f) / 48.0f;
    if (beatPulse_) {
      sPos_ = sizeBeat_;
    }
  }

  const float step = scaledStep(context.deltaSeconds);
  velocity_[0] -= kSpringStrength * step * (position_[0] - target_[0]);
  velocity_[1] -= kSpringStrength * step * (position_[1] - target_[1]);

  position_[0] += velocity_[0] * step;
  position_[1] += velocity_[1] * step;

  const float damping = std::pow(kDamping, step);
  velocity_[0] *= damping;
  velocity_[1] *= damping;

  const int ss = std::min(context.height / 2, (context.width * 3) / 8);
  if (ss <= 0) {
    return true;
  }

  const float radiusScale = static_cast<float>(ss) * (static_cast<float>(maxDistance_) / 32.0f);
  const int xp =
      static_cast<int>(position_[0] * radiusScale + static_cast<float>(context.width) * 0.5f);
  const int yp =
      static_cast<int>(position_[1] * radiusScale + static_cast<float>(context.height) * 0.5f);

  int sz = std::clamp(sPos_, kMinRadius, kMaxRadius);
  sPos_ = (sPos_ + sizeBase_) / 2;
  if (sPos_ < kMinRadius) {
    sPos_ = kMinRadius;
  }

  std::uint8_t* framebuffer = context.framebuffer.data;
  const int width = context.width;
  const int height = context.height;

  const auto drawPixel = [&](int x, int y) {
    if (x < 0 || y < 0 || x >= width || y >= height) {
      return;
    }
    const std::size_t offset = (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                                static_cast<std::size_t>(x)) *
                               4u;
    std::uint8_t* pixel = framebuffer + offset;
    applyBlend(pixel);
  };

  if (sz <= 1) {
    drawPixel(xp, yp);
    return true;
  }

  const double radius = static_cast<double>(sz) * 0.5;
  const int yStart = yp - sz / 2;
  for (int row = 0; row < sz; ++row) {
    const int y = yStart + row;
    if (y < 0 || y >= height) {
      continue;
    }
    const double yOffset = static_cast<double>(row) - radius;
    const double span = std::sqrt(std::max(radius * radius - yOffset * yOffset, 0.0));
    int xSpan = static_cast<int>(span + 0.99);
    if (xSpan < 1) {
      xSpan = 1;
    }
    int xStart = xp - xSpan;
    int xEnd = xp + xSpan;
    if (xStart < 0) {
      xStart = 0;
    }
    if (xEnd > width) {
      xEnd = width;
    }
    for (int x = xStart; x < xEnd; ++x) {
      drawPixel(x, y);
    }
  }

  return true;
}
