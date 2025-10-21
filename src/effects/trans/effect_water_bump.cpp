#include "effects/trans/effect_water_bump.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "avs/core/RenderContext.hpp"

namespace avs::effects::trans {

namespace {
constexpr int kMinDimensionForSimulation = 3;
constexpr int kMaxDamping = 10;
constexpr int kMaxDropRadius = 256;
constexpr int kMaxDepth = 4000;
constexpr double kCosineScale = 65535.0;

int clampInt(int value, int minValue, int maxValue) {
  return value < minValue ? minValue : (value > maxValue ? maxValue : value);
}

}  // namespace

bool WaterBump::hasFramebuffer(const avs::core::RenderContext& context) const {
  if (!context.framebuffer.data || context.width <= 0 || context.height <= 0) {
    return false;
  }
  const std::size_t expected =
      static_cast<std::size_t>(context.width) * static_cast<std::size_t>(context.height) * 4u;
  return context.framebuffer.size >= expected;
}

void WaterBump::setParams(const avs::core::ParamBlock& params) {
  enabled_ = params.getBool("enabled", enabled_);
  if (params.contains("damping")) {
    damping_ = clampInt(params.getInt("damping", damping_), 0, kMaxDamping);
  } else if (params.contains("density")) {
    damping_ = clampInt(params.getInt("density", damping_), 0, kMaxDamping);
  } else {
    damping_ = clampInt(damping_, 0, kMaxDamping);
  }
  depth_ = clampInt(params.getInt("depth", depth_), 0, kMaxDepth);
  randomDrop_ = params.getBool("random_drop", randomDrop_);
  dropPosX_ = clampInt(params.getInt("drop_position_x", dropPosX_), 0, 2);
  dropPosY_ = clampInt(params.getInt("drop_position_y", dropPosY_), 0, 2);
  dropRadius_ = clampInt(params.getInt("drop_radius", dropRadius_), 0, kMaxDropRadius);
  method_ = params.getInt("method", method_);
}

void WaterBump::resetBuffers(int width, int height) {
  bufferWidth_ = width;
  bufferHeight_ = height;
  currentPage_ = 0;
  const std::size_t total = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
  for (auto& buffer : heightBuffers_) {
    buffer.assign(total, 0);
  }
}

int WaterBump::clampCenter(int value, int radius, int dimension) {
  if (dimension <= 0) {
    return 0;
  }
  const int minCenter = std::max(1, radius + 1);
  const int maxCenter = std::max(minCenter, dimension - radius - 1);
  return clampInt(value, minCenter, maxCenter);
}

int WaterBump::randomInRange(avs::core::DeterministicRng& rng, int minInclusive, int maxInclusive) {
  if (maxInclusive < minInclusive) {
    return minInclusive;
  }
  const std::uint32_t span = static_cast<std::uint32_t>(maxInclusive - minInclusive + 1);
  if (span == 0) {
    return minInclusive;
  }
  const std::uint32_t sample = rng.nextUint32();
  return minInclusive + static_cast<int>(sample % span);
}

void WaterBump::addSineBlob(int centerX, int centerY, int radius, int amplitude) {
  if (radius <= 0 || amplitude == 0 || bufferWidth_ < kMinDimensionForSimulation ||
      bufferHeight_ < kMinDimensionForSimulation) {
    return;
  }

  radius = clampInt(radius, 1, std::min(bufferWidth_, bufferHeight_) - 2);
  centerX = clampCenter(centerX, radius, bufferWidth_);
  centerY = clampCenter(centerY, radius, bufferHeight_);

  auto& current = heightBuffers_[currentPage_];
  const int radiusSquared = radius * radius;
  const double length =
      (1024.0 / static_cast<double>(radius)) * (1024.0 / static_cast<double>(radius));

  const int startY = centerY - radius;
  const int endY = centerY + radius;
  for (int y = startY; y < endY; ++y) {
    const int dy = y - centerY;
    const int rowOffset = y * bufferWidth_;
    for (int x = centerX - radius; x < centerX + radius; ++x) {
      const int dx = x - centerX;
      const int distanceSquared = dx * dx + dy * dy;
      if (distanceSquared >= radiusSquared) {
        continue;
      }
      const double dist = std::sqrt(static_cast<double>(distanceSquared) * length);
      const double raw = (std::cos(dist) + kCosineScale) * static_cast<double>(amplitude);
      const int delta = static_cast<int>(raw) >> 19;
      current[rowOffset + x] += delta;
    }
  }
}

void WaterBump::applyBeatDrop(avs::core::RenderContext& context) {
  if (depth_ <= 0) {
    return;
  }

  int radius = dropRadius_;
  int centerX = bufferWidth_ / 2;
  int centerY = bufferHeight_ / 2;

  if (randomDrop_) {
    const int maxDimension = std::max(bufferWidth_, bufferHeight_);
    radius = (dropRadius_ * maxDimension) / 100;
    if (radius <= 0) {
      radius = std::max(1, dropRadius_);
    }
    const int minX = std::max(1, radius + 1);
    const int maxX = std::max(minX, bufferWidth_ - radius - 1);
    const int minY = std::max(1, radius + 1);
    const int maxY = std::max(minY, bufferHeight_ - radius - 1);
    centerX = randomInRange(context.rng, minX, maxX);
    centerY = randomInRange(context.rng, minY, maxY);
  } else {
    const int xPositions[3] = {bufferWidth_ / 4, bufferWidth_ / 2, (bufferWidth_ * 3) / 4};
    const int yPositions[3] = {bufferHeight_ / 4, bufferHeight_ / 2, (bufferHeight_ * 3) / 4};
    centerX = xPositions[clampInt(dropPosX_, 0, 2)];
    centerY = yPositions[clampInt(dropPosY_, 0, 2)];
  }

  addSineBlob(centerX, centerY, radius, -depth_);
}

void WaterBump::updateWater(int nextPage) {
  if (bufferWidth_ < kMinDimensionForSimulation || bufferHeight_ < kMinDimensionForSimulation) {
    heightBuffers_[nextPage] = heightBuffers_[currentPage_];
    return;
  }

  auto& newBuffer = heightBuffers_[nextPage];
  const auto& oldBuffer = heightBuffers_[currentPage_];
  const int width = bufferWidth_;
  const int height = bufferHeight_;

  for (int y = 1; y < height - 1; ++y) {
    const int row = y * width;
    for (int x = 1; x < width - 1; ++x) {
      const int idx = row + x;
      int newh = oldBuffer[idx + width] + oldBuffer[idx - width] + oldBuffer[idx + 1] +
                 oldBuffer[idx - 1] + oldBuffer[idx - width - 1] + oldBuffer[idx - width + 1] +
                 oldBuffer[idx + width - 1] + oldBuffer[idx + width + 1];
      newh = (newh >> 2) - newBuffer[idx];
      newBuffer[idx] = newh - (damping_ > 0 ? (newh >> damping_) : newh);
    }
  }

  for (int x = 0; x < width; ++x) {
    newBuffer[x] = 0;
    newBuffer[(height - 1) * width + x] = 0;
  }
  for (int y = 0; y < height; ++y) {
    newBuffer[y * width] = 0;
    newBuffer[y * width + (width - 1)] = 0;
  }
}

bool WaterBump::render(avs::core::RenderContext& context) {
  if (!enabled_) {
    return true;
  }
  if (!hasFramebuffer(context)) {
    return true;
  }

  const int width = context.width;
  const int height = context.height;
  if (width < kMinDimensionForSimulation || height < kMinDimensionForSimulation) {
    return true;
  }

  if (width != bufferWidth_ || height != bufferHeight_) {
    resetBuffers(width, height);
  }

  const std::size_t bytes = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u;
  if (scratch_.size() < bytes) {
    scratch_.resize(bytes);
  }
  std::memcpy(scratch_.data(), context.framebuffer.data, bytes);

  if (context.audioBeat) {
    applyBeatDrop(context);
  }

  const auto& current = heightBuffers_[currentPage_];
  std::uint8_t* dest = context.framebuffer.data;
  const std::uint8_t* source = scratch_.data();
  const int totalPixels = width * height;

  for (int y = 1; y < height - 1; ++y) {
    const int row = y * width;
    for (int x = 1; x < width - 1; ++x) {
      const int idx = row + x;
      const int dx = current[idx] - current[idx + 1];
      const int dy = current[idx] - current[idx + width];
      const int sample = idx + (dy >> 3) * width + (dx >> 3);
      if (sample >= 0 && sample < totalPixels) {
        std::memcpy(dest + static_cast<std::size_t>(idx) * 4u,
                    source + static_cast<std::size_t>(sample) * 4u, 4u);
      }
    }
  }

  const int nextPage = (currentPage_ + 1) & 1;
  updateWater(nextPage);
  currentPage_ = nextPage;

  return true;
}

}  // namespace avs::effects::trans
