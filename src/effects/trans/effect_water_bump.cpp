#include "effects/trans/effect_water_bump.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <limits>

namespace avs::effects::trans {
namespace {

constexpr std::size_t kChannels = 4u;
constexpr float kPi = 3.14159265358979323846f;

bool hasFramebuffer(const avs::core::RenderContext& context, std::size_t requiredBytes) {
  return context.framebuffer.data != nullptr && context.framebuffer.size >= requiredBytes &&
         context.width > 0 && context.height > 0;
}

}  // namespace

WaterBump::WaterBump() = default;

bool WaterBump::render(avs::core::RenderContext& context) {
  if (!enabled_) {
    return true;
  }

  if (context.width <= 0 || context.height <= 0) {
    return true;
  }

  const std::size_t width = static_cast<std::size_t>(context.width);
  const std::size_t height = static_cast<std::size_t>(context.height);

  const std::size_t requiredBytes = width * height * kChannels;
  if (!hasFramebuffer(context, requiredBytes)) {
    return true;
  }

  ensureCapacity(width, height);

  if (context.audioBeat) {
    addDrop(context);
  }

  applyWaterBump(context);
  propagateWave();

  return true;
}

void WaterBump::setParams(const avs::core::ParamBlock& params) {
  if (params.contains("enabled")) {
    enabled_ = params.getBool("enabled", enabled_);
  }

  const auto parseInt = [&params](const std::initializer_list<const char*>& keys, int fallback) {
    constexpr int kSentinel = std::numeric_limits<int>::min();
    for (const char* key : keys) {
      if (!params.contains(key)) {
        continue;
      }
      const int intValue = params.getInt(key, kSentinel);
      if (intValue != kSentinel) {
        return intValue;
      }
      const bool boolValue = params.getBool(key, fallback != 0);
      return boolValue ? 1 : 0;
    }
    return fallback;
  };

  densityShift_ = std::clamp(parseInt({"density", "damp", "damping"}, densityShift_), 0, 10);
  depth_ = std::clamp(parseInt({"depth", "amplitude"}, depth_), 0, 4000);
  randomDrop_ = parseInt({"random_drop", "randomdrop"}, randomDrop_ ? 1 : 0) != 0;

  dropRadiusPercent_ =
      std::clamp(parseInt({"drop_radius", "radius", "dropradius"}, dropRadiusPercent_), 0, 100);

  const int dropX = parseInt({"drop_position_x", "dropx"}, static_cast<int>(dropPositionX_));
  const int dropY = parseInt({"drop_position_y", "dropy"}, static_cast<int>(dropPositionY_));
  dropPositionX_ = clampDropPosition(dropX);
  dropPositionY_ = clampDropPosition(dropY);

  method_ = parseInt({"method"}, method_);
}

void WaterBump::ensureCapacity(std::size_t width, std::size_t height) {
  const bool resolutionChanged =
      width_ != static_cast<int>(width) || height_ != static_cast<int>(height);
  const std::size_t elementCount = width * height;

  if (resolutionChanged) {
    width_ = static_cast<int>(width);
    height_ = static_cast<int>(height);
    frontBuffer_ = 0;
  }

  for (auto& buffer : heightBuffers_) {
    if (buffer.size() != elementCount || resolutionChanged) {
      buffer.assign(elementCount, 0.0f);
    }
  }

  const std::size_t requiredBytes = elementCount * kChannels;
  if (scratch_.size() != requiredBytes || resolutionChanged) {
    scratch_.assign(requiredBytes, 0u);
  }
}

void WaterBump::addDrop(const avs::core::RenderContext& context) {
  if (width_ <= 2 || height_ <= 2) {
    return;
  }

  const float radiusPixels = computeRadiusPixels();
  if (radiusPixels <= 1.0f) {
    return;
  }

  int centerX = 0;
  int centerY = 0;

  const float minX = std::max(radiusPixels, 1.0f);
  const float maxX = static_cast<float>(width_ - 1) - radiusPixels;
  const float minY = std::max(radiusPixels, 1.0f);
  const float maxY = static_cast<float>(height_ - 1) - radiusPixels;

  if (randomDrop_) {
    const bool hasRandomX = maxX > minX;
    const bool hasRandomY = maxY > minY;
    if (hasRandomX) {
      const float rx = context.rng.uniform(minX, maxX);
      centerX = static_cast<int>(std::lround(std::clamp(rx, minX, maxX)));
    } else {
      centerX = width_ / 2;
    }
    if (hasRandomY) {
      const float ry = context.rng.uniform(minY, maxY);
      centerY = static_cast<int>(std::lround(std::clamp(ry, minY, maxY)));
    } else {
      centerY = height_ / 2;
    }
  } else {
    centerX = computeDropCoordinate(dropPositionX_, width_);
    centerY = computeDropCoordinate(dropPositionY_, height_);
    const int lowerX = static_cast<int>(std::ceil(minX));
    const int upperX = static_cast<int>(std::floor(maxX));
    const int lowerY = static_cast<int>(std::ceil(minY));
    const int upperY = static_cast<int>(std::floor(maxY));
    centerX = (lowerX <= upperX) ? std::clamp(centerX, lowerX, upperX) : width_ / 2;
    centerY = (lowerY <= upperY) ? std::clamp(centerY, lowerY, upperY) : height_ / 2;
  }

  addSineBlob(centerX, centerY, radiusPixels, -static_cast<float>(depth_));
}

void WaterBump::addSineBlob(int centerX, int centerY, float radius, float amplitude) {
  if (heightBuffers_[frontBuffer_].empty() || radius <= 1.0f) {
    return;
  }

  const float radiusSq = radius * radius;
  const int minX = std::max(1, static_cast<int>(std::floor(static_cast<float>(centerX) - radius)));
  const int maxX =
      std::min(width_ - 2, static_cast<int>(std::ceil(static_cast<float>(centerX) + radius)));
  const int minY = std::max(1, static_cast<int>(std::floor(static_cast<float>(centerY) - radius)));
  const int maxY =
      std::min(height_ - 2, static_cast<int>(std::ceil(static_cast<float>(centerY) + radius)));

  auto& buffer = heightBuffers_[frontBuffer_];

  for (int y = minY; y <= maxY; ++y) {
    const float dy = static_cast<float>(y - centerY);
    for (int x = minX; x <= maxX; ++x) {
      const float dx = static_cast<float>(x - centerX);
      const float distSq = dx * dx + dy * dy;
      if (distSq > radiusSq) {
        continue;
      }

      const float normalized = std::sqrt(distSq) / radius;
      const float influence = std::cos(std::min(normalized, 1.0f) * kPi);
      const std::size_t index = static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) +
                                static_cast<std::size_t>(x);
      buffer[index] += amplitude * influence;
    }
  }
}

void WaterBump::propagateWave() {
  if (width_ <= 2 || height_ <= 2) {
    return;
  }

  auto& current = heightBuffers_[frontBuffer_];
  auto& next = heightBuffers_[frontBuffer_ ^ 1];

  const int w = width_;
  const int h = height_;
  const int densityShift = std::clamp(densityShift_, 0, 10);
  const float attenuation = 1.0f / static_cast<float>(1 << densityShift);

  for (int y = 1; y < h - 1; ++y) {
    const int row = y * w;
    for (int x = 1; x < w - 1; ++x) {
      const int idx = row + x;
      const float neighborSum = current[idx - w] + current[idx + w] + current[idx - 1] +
                                current[idx + 1] + current[idx - w - 1] + current[idx - w + 1] +
                                current[idx + w - 1] + current[idx + w + 1];
      const float newHeight = 0.25f * neighborSum - next[idx];
      next[idx] = newHeight - newHeight * attenuation;
    }
  }

  for (int x = 0; x < w; ++x) {
    next[x] = 0.0f;
    next[(h - 1) * w + x] = 0.0f;
  }
  for (int y = 0; y < h; ++y) {
    next[y * w] = 0.0f;
    next[y * w + (w - 1)] = 0.0f;
  }

  frontBuffer_ ^= 1;
}

void WaterBump::applyWaterBump(avs::core::RenderContext& context) {
  if (width_ <= 0 || height_ <= 0) {
    return;
  }

  const auto& heightMap = heightBuffers_[frontBuffer_];
  const std::uint8_t* src = context.framebuffer.data;
  std::uint8_t* dst = scratch_.data();

  const float displacementScale = 0.03f * (static_cast<float>(depth_) / 600.0f);
  const float normalScale = 0.45f;
  const float maxOffset = 32.0f;

  constexpr float lightX = -0.4f;
  constexpr float lightY = -0.6f;
  constexpr float lightZ = 1.0f;
  const float lightLength = std::sqrt(lightX * lightX + lightY * lightY + lightZ * lightZ);
  const float lx = lightX / lightLength;
  const float ly = lightY / lightLength;
  const float lz = lightZ / lightLength;

  const std::size_t width = static_cast<std::size_t>(width_);
  const std::size_t height = static_cast<std::size_t>(height_);
  const std::size_t requiredBytes = width * height * kChannels;

  for (int y = 0; y < height_; ++y) {
    for (int x = 0; x < width_; ++x) {
      const std::size_t index = static_cast<std::size_t>(y) * width + static_cast<std::size_t>(x);

      const std::size_t leftIndex = (x > 0) ? index - 1 : index;
      const std::size_t rightIndex = (x < width_ - 1) ? index + 1 : index;
      const std::size_t upIndex = (y > 0) ? index - width : index;
      const std::size_t downIndex = (y < height_ - 1) ? index + width : index;

      const float dx = heightMap[rightIndex] - heightMap[leftIndex];
      const float dy = heightMap[downIndex] - heightMap[upIndex];

      const float offsetX = std::clamp(dx * displacementScale, -maxOffset, maxOffset);
      const float offsetY = std::clamp(dy * displacementScale, -maxOffset, maxOffset);
      const int sampleX =
          std::clamp(static_cast<int>(std::lround(static_cast<float>(x) + offsetX)), 0, width_ - 1);
      const int sampleY = std::clamp(static_cast<int>(std::lround(static_cast<float>(y) + offsetY)),
                                     0, height_ - 1);

      const std::size_t sampleIndex =
          (static_cast<std::size_t>(sampleY) * width + static_cast<std::size_t>(sampleX)) *
          kChannels;
      const std::size_t dstIndex = index * kChannels;

      float nx = -dx * normalScale;
      float ny = -dy * normalScale;
      float nz = 1.0f;
      const float invLength = 1.0f / std::sqrt(nx * nx + ny * ny + nz * nz);
      nx *= invLength;
      ny *= invLength;
      nz *= invLength;

      float shade = nx * lx + ny * ly + nz * lz;
      shade = std::clamp(shade, 0.0f, 1.0f);
      const float brightness = 0.55f + 0.45f * shade;

      dst[dstIndex + 0] =
          clampByte(static_cast<int>(std::lround(src[sampleIndex + 0] * brightness)));
      dst[dstIndex + 1] =
          clampByte(static_cast<int>(std::lround(src[sampleIndex + 1] * brightness)));
      dst[dstIndex + 2] =
          clampByte(static_cast<int>(std::lround(src[sampleIndex + 2] * brightness)));
      dst[dstIndex + 3] = src[sampleIndex + 3];
    }
  }

  std::memcpy(context.framebuffer.data, scratch_.data(), requiredBytes);
}

float WaterBump::computeRadiusPixels() const {
  if (width_ <= 0 || height_ <= 0) {
    return 0.0f;
  }

  const float percent = static_cast<float>(std::clamp(dropRadiusPercent_, 0, 100)) / 100.0f;
  if (percent <= 0.0f) {
    return 0.0f;
  }

  const float base = randomDrop_ ? static_cast<float>(std::max(width_, height_))
                                 : static_cast<float>(std::min(width_, height_));
  const float radius = base * percent;
  const float maxRadius = std::max(2.0f, base * 0.5f);
  return std::clamp(radius, 2.0f, maxRadius);
}

int WaterBump::computeDropCoordinate(DropAxisPosition position, int extent) const {
  switch (position) {
    case DropAxisPosition::Left:
      return extent / 4;
    case DropAxisPosition::Right:
      return (extent * 3) / 4;
    case DropAxisPosition::Center:
    default:
      return extent / 2;
  }
}

WaterBump::DropAxisPosition WaterBump::clampDropPosition(int value) {
  if (value <= 0) {
    return DropAxisPosition::Left;
  }
  if (value == 1) {
    return DropAxisPosition::Center;
  }
  if (value >= 2) {
    return DropAxisPosition::Right;
  }
  return DropAxisPosition::Center;
}

std::uint8_t WaterBump::clampByte(int value) {
  return static_cast<std::uint8_t>(std::clamp(value, 0, 255));
}

}  // namespace avs::effects::trans
