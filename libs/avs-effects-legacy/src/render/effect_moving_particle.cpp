#include <avs/effects/render/effect_moving_particle.h>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>

#include <avs/audio/analyzer.h>

namespace avs::effects::render {
namespace {

constexpr double kAttraction = 0.004;
constexpr double kVelocityDamping = 0.991;

}  // namespace

MovingParticle::MovingParticle() {
  color_ = colorFromInt(0xFFFFFFu);
  target_[0] = 0.0;
  target_[1] = 0.0;
  velocity_[0] = -0.01551;
  velocity_[1] = 0.0;
  position_[0] = -0.6;
  position_[1] = 0.3;
}

bool MovingParticle::hasFramebuffer(const avs::core::RenderContext& context) {
  if (!context.framebuffer.data || context.width <= 0 || context.height <= 0) {
    return false;
  }
  const std::size_t required =
      static_cast<std::size_t>(context.width) * static_cast<std::size_t>(context.height) * 4u;
  return context.framebuffer.size >= required;
}

MovingParticle::Color MovingParticle::colorFromInt(std::uint32_t value) {
  Color color{};
  color.r = static_cast<std::uint8_t>((value >> 16u) & 0xFFu);
  color.g = static_cast<std::uint8_t>((value >> 8u) & 0xFFu);
  color.b = static_cast<std::uint8_t>(value & 0xFFu);
  color.a = 255u;
  return color;
}

MovingParticle::Color MovingParticle::colorFromString(const std::string& value, Color fallback) {
  if (value.empty()) {
    return fallback;
  }
  std::string token;
  for (char ch : value) {
    if (!std::isspace(static_cast<unsigned char>(ch)) && ch != ',' && ch != ';') {
      token.push_back(ch);
    }
  }
  if (token.empty()) {
    return fallback;
  }
  std::string_view view(token);
  int base = 10;
  if (view.front() == '#') {
    view.remove_prefix(1);
    base = 16;
  } else if (view.size() > 2 && view.front() == '0' && (view[1] == 'x' || view[1] == 'X')) {
    view.remove_prefix(2);
    base = 16;
  } else {
    bool allHex = true;
    for (char ch : view) {
      if (!std::isxdigit(static_cast<unsigned char>(ch))) {
        allHex = false;
        break;
      }
    }
    if (allHex) {
      base = 16;
    }
  }
  std::uint32_t parsed = 0;
  auto result = std::from_chars(view.data(), view.data() + view.size(), parsed, base);
  if (result.ec != std::errc() || result.ptr != view.data() + view.size()) {
    return fallback;
  }
  return colorFromInt(parsed);
}

std::uint8_t MovingParticle::saturatingAdd(std::uint8_t base, std::uint8_t add) {
  const int sum = static_cast<int>(base) + static_cast<int>(add);
  return static_cast<std::uint8_t>(std::min(sum, 255));
}

void MovingParticle::applyBlend(std::uint8_t* pixel, const Color& color, int mode) {
  switch (mode) {
    case 0:
      pixel[0] = color.r;
      pixel[1] = color.g;
      pixel[2] = color.b;
      break;
    case 2:
      pixel[0] = static_cast<std::uint8_t>((static_cast<int>(pixel[0]) + color.r) / 2);
      pixel[1] = static_cast<std::uint8_t>((static_cast<int>(pixel[1]) + color.g) / 2);
      pixel[2] = static_cast<std::uint8_t>((static_cast<int>(pixel[2]) + color.b) / 2);
      break;
    case 3:
      pixel[0] = std::max(pixel[0], color.r);
      pixel[1] = std::max(pixel[1], color.g);
      pixel[2] = std::max(pixel[2], color.b);
      break;
    case 1:
    default:
      pixel[0] = saturatingAdd(pixel[0], color.r);
      pixel[1] = saturatingAdd(pixel[1], color.g);
      pixel[2] = saturatingAdd(pixel[2], color.b);
      break;
  }
  pixel[3] = 255u;
}

void MovingParticle::parseParams(const avs::core::ParamBlock& params) {
  if (params.contains("enabled")) {
    enabledMask_ = params.getInt("enabled", enabledMask_) & 0b11;
  }
  if (params.contains("color")) {
    color_ = colorFromInt(static_cast<std::uint32_t>(params.getInt("color", 0)));
  }
  if (params.contains("colour")) {
    color_ = colorFromString(params.getString("colour", ""), color_);
  }
  if (params.contains("colors")) {
    color_ = colorFromString(params.getString("colors", ""), color_);
  }
  maxDistance_ = std::clamp(params.getInt("maxdist", maxDistance_), 1, 128);
  baseSize_ = std::clamp(params.getInt("size", baseSize_), 1, kMaxSize);
  beatSize_ = std::clamp(params.getInt("size2", beatSize_), 1, kMaxSize);
  blendMode_ = std::clamp(params.getInt("blend", blendMode_), 0, 3);
  sizePosition_ = std::clamp(sizePosition_, 1, kMaxSize);
}

int MovingParticle::randomInt(avs::core::RenderContext& context, int min, int max) {
  if (min > max) {
    std::swap(min, max);
  }
  const std::uint32_t value = context.rng.nextUint32();
  const std::uint32_t range = static_cast<std::uint32_t>(max - min + 1);
  return min + static_cast<int>(value % range);
}

void MovingParticle::drawCircle(avs::core::RenderContext& context, int centerX, int centerY,
                                int diameter) const {
  const int width = context.width;
  const int height = context.height;
  std::uint8_t* framebuffer = context.framebuffer.data;
  const double radius = static_cast<double>(diameter) * 0.5;
  const double radiusSquared = radius * radius;
  int top = centerY - diameter / 2;

  for (int y = 0; y < diameter; ++y) {
    const int scanY = top + y;
    if (scanY < 0 || scanY >= height) {
      continue;
    }
    const double yOffset = static_cast<double>(y) - radius;
    const double spanSquared = radiusSquared - yOffset * yOffset;
    if (spanSquared <= 0.0) {
      continue;
    }
    const double length = std::sqrt(spanSquared);
    int xs = static_cast<int>(std::floor(length + 0.99));
    if (xs < 1) {
      xs = 1;
    }
    int start = centerX - xs;
    int end = centerX + xs;
    if (start < 0) {
      start = 0;
    }
    if (end >= width) {
      end = width - 1;
    }
    const std::size_t rowOffset = static_cast<std::size_t>(scanY) * static_cast<std::size_t>(width);
    for (int x = start; x <= end; ++x) {
      const std::size_t index = (rowOffset + static_cast<std::size_t>(x)) * 4u;
      applyBlend(framebuffer + index, color_, blendMode_);
    }
  }
}

void MovingParticle::setParams(const avs::core::ParamBlock& params) { parseParams(params); }

bool MovingParticle::render(avs::core::RenderContext& context) {
  if (!hasFramebuffer(context) || (enabledMask_ & 1) == 0) {
    return true;
  }

  if (context.audioBeat) {
    target_[0] = static_cast<double>(randomInt(context, -16, 16)) / 48.0;
    target_[1] = static_cast<double>(randomInt(context, -16, 16)) / 48.0;
    if ((enabledMask_ & 2) != 0) {
      sizePosition_ = beatSize_;
    }
  }

  velocity_[0] -= kAttraction * (position_[0] - target_[0]);
  velocity_[1] -= kAttraction * (position_[1] - target_[1]);

  position_[0] += velocity_[0];
  position_[1] += velocity_[1];

  velocity_[0] *= kVelocityDamping;
  velocity_[1] *= kVelocityDamping;

  const int ss = std::min(context.height / 2, (context.width * 3) / 8);
  if (ss <= 0) {
    return true;
  }

  const double scale = static_cast<double>(ss) * static_cast<double>(maxDistance_) / 32.0;
  const int posX = static_cast<int>(std::trunc(position_[0] * scale)) + context.width / 2;
  const int posY = static_cast<int>(std::trunc(position_[1] * scale)) + context.height / 2;

  int currentSize = std::clamp(sizePosition_, 1, kMaxSize);
  sizePosition_ = (sizePosition_ + baseSize_) / 2;
  if (sizePosition_ < 1) {
    sizePosition_ = 1;
  }

  if (currentSize <= 1) {
    if (posX >= 0 && posX < context.width && posY >= 0 && posY < context.height) {
      const std::size_t index =
          (static_cast<std::size_t>(posY) * static_cast<std::size_t>(context.width) +
           static_cast<std::size_t>(posX)) *
          4u;
      applyBlend(context.framebuffer.data + index, color_, blendMode_);
    }
    return true;
  }

  if (currentSize > 128) {
    currentSize = 128;
  }
  drawCircle(context, posX, posY, currentSize);
  return true;
}

}  // namespace avs::effects::render
