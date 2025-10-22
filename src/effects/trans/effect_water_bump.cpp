#include "effects/trans/effect_water_bump.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "avs/core/ParamBlock.hpp"

namespace avs::effects::trans {

namespace {
constexpr std::size_t kChannels = 4u;
constexpr int kMinRadius = 1;
}

int WaterBump::clampDensity(int value) {
  return std::clamp(value, 1, 16);
}

int WaterBump::randomInRange(avs::core::DeterministicRng& rng,
                             int minInclusive,
                             int maxInclusive) {
  if (maxInclusive <= minInclusive) {
    return minInclusive;
  }
  const std::uint32_t span = static_cast<std::uint32_t>(maxInclusive - minInclusive + 1);
  return minInclusive + static_cast<int>(rng.nextUint32() % span);
}

bool WaterBump::ensureBuffers(int width, int height) {
  if (width <= 0 || height <= 0) {
    width_ = 0;
    height_ = 0;
    page_ = 0;
    for (auto& buffer : heightMaps_) {
      buffer.clear();
    }
    sourceFrame_.clear();
    return false;
  }

  if (width == width_ && height == height_) {
    return true;
  }

  width_ = width;
  height_ = height;
  page_ = 0;

  const std::size_t totalPixels = static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_);
  for (auto& buffer : heightMaps_) {
    buffer.assign(totalPixels, 0);
  }
  sourceFrame_.assign(totalPixels * kChannels, 0u);
  return true;
}

void WaterBump::setParams(const avs::core::ParamBlock& params) {
  if (params.contains("enabled")) {
    enabled_ = params.getBool("enabled", enabled_);
  }
  if (params.contains("density")) {
    density_ = clampDensity(params.getInt("density", density_));
  }
  if (params.contains("depth")) {
    depth_ = params.getInt("depth", depth_);
  }
  if (params.contains("random_drop")) {
    randomDrop_ = params.getBool("random_drop", randomDrop_);
  }
  if (params.contains("drop_position_x")) {
    dropPositionX_ = std::clamp(params.getInt("drop_position_x", dropPositionX_), 0, 2);
  }
  if (params.contains("drop_position_y")) {
    dropPositionY_ = std::clamp(params.getInt("drop_position_y", dropPositionY_), 0, 2);
  }
  if (params.contains("drop_radius")) {
    dropRadius_ = std::max(kMinRadius, params.getInt("drop_radius", dropRadius_));
  }
  if (params.contains("method")) {
    method_ = params.getInt("method", method_);
  }
}

void WaterBump::sineBlob(int x,
                         int y,
                         int radius,
                         int height,
                         avs::core::DeterministicRng& rng) {
  if (radius < kMinRadius || width_ <= 2 || height_ <= 2) {
    return;
  }

  radius = std::min(radius, std::min(width_, height_) - 1);
  if (radius < kMinRadius) {
    return;
  }

  const int interiorWidth = width_ - 2 * radius - 1;
  const int interiorHeight = height_ - 2 * radius - 1;

  if (x < 0 && interiorWidth > 0) {
    x = radius + 1 + randomInRange(rng, 0, interiorWidth);
  }
  if (y < 0 && interiorHeight > 0) {
    y = radius + 1 + randomInRange(rng, 0, interiorHeight);
  }

  x = std::clamp(x, radius + 1, width_ - radius - 1);
  y = std::clamp(y, radius + 1, height_ - radius - 1);

  const int left = -radius;
  const int right = radius;
  const int top = -radius;
  const int bottom = radius;

  const int radiusSquared = radius * radius;
  const double length = (1024.0 / static_cast<double>(radius)) *
                        (1024.0 / static_cast<double>(radius));

  auto& buffer = heightMaps_[static_cast<std::size_t>(page_)];

  for (int cy = top; cy < bottom; ++cy) {
    const int yy = cy + y;
    if (yy <= 0 || yy >= height_ - 1) {
      continue;
    }
    for (int cx = left; cx < right; ++cx) {
      const int square = cy * cy + cx * cx;
      if (square >= radiusSquared) {
        continue;
      }
      const int xx = cx + x;
      if (xx <= 0 || xx >= width_ - 1) {
        continue;
      }
      const double dist = std::sqrt(static_cast<double>(square) * length);
      const double contribution = (std::cos(dist) + 65535.0) * static_cast<double>(height);
      const int delta = static_cast<int>(contribution / 524288.0);
      const std::size_t index = static_cast<std::size_t>(yy) * static_cast<std::size_t>(width_) +
                                static_cast<std::size_t>(xx);
      buffer[index] += delta;
    }
  }
}

void WaterBump::calcWater(int nextPage) {
  if (width_ <= 2 || height_ <= 2) {
    return;
  }

  auto& newBuffer = heightMaps_[static_cast<std::size_t>(nextPage)];
  const auto& oldBuffer = heightMaps_[static_cast<std::size_t>(page_)];
  const int totalWidth = width_;
  const int totalHeight = height_;

  for (int y = 1; y < totalHeight - 1; ++y) {
    for (int x = 1; x < totalWidth - 1; ++x) {
      const std::size_t index = static_cast<std::size_t>(y) * static_cast<std::size_t>(totalWidth) +
                                static_cast<std::size_t>(x);
      int newh = oldBuffer[index + totalWidth] + oldBuffer[index - totalWidth] +
                 oldBuffer[index + 1] + oldBuffer[index - 1] +
                 oldBuffer[index - totalWidth - 1] + oldBuffer[index - totalWidth + 1] +
                 oldBuffer[index + totalWidth - 1] + oldBuffer[index + totalWidth + 1];
      newh >>= 2;
      newh -= newBuffer[index];
      newBuffer[index] = newh - (newh >> density_);
    }
  }
}

bool WaterBump::render(avs::core::RenderContext& context) {
  if (!enabled_) {
    return true;
  }

  if (context.width <= 0 || context.height <= 0) {
    return true;
  }

  const int width = context.width;
  const int height = context.height;
  const std::size_t totalPixels = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
  const std::size_t requiredBytes = totalPixels * kChannels;
  if (!context.framebuffer.data || context.framebuffer.size < requiredBytes) {
    return true;
  }

  if (!ensureBuffers(width, height)) {
    return true;
  }

  std::uint8_t* framebuffer = context.framebuffer.data;
  std::memcpy(sourceFrame_.data(), framebuffer, requiredBytes);

  if (context.audioBeat) {
    const int dropHeight = -std::abs(depth_);
    if (randomDrop_) {
      const int maxDimension = std::max(width, height);
      int radius = std::max(kMinRadius, dropRadius_ * maxDimension / 100);
      sineBlob(-1, -1, radius, dropHeight, context.rng);
    } else {
      int x = width / 2;
      int y = height / 2;
      if (dropPositionX_ == 0) {
        x = width / 4;
      } else if (dropPositionX_ == 2) {
        x = (width * 3) / 4;
      }
      if (dropPositionY_ == 0) {
        y = height / 4;
      } else if (dropPositionY_ == 2) {
        y = (height * 3) / 4;
      }
      sineBlob(x, y, std::max(kMinRadius, dropRadius_), dropHeight, context.rng);
    }
  }

  const auto& currentBuffer = heightMaps_[static_cast<std::size_t>(page_)];
  auto* destination = framebuffer;
  const auto* source = sourceFrame_.data();

  if (width <= 1 || height <= 1) {
    std::memcpy(destination, source, requiredBytes);
    return true;
  }

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const std::size_t index = static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                                static_cast<std::size_t>(x);
      const std::size_t pixelOffset = index * kChannels;

      if (x >= width - 1 || y >= height - 1 || x == 0 || y == 0) {
        std::memcpy(destination + pixelOffset, source + pixelOffset, kChannels);
        continue;
      }

      const int dx = currentBuffer[index] - currentBuffer[index + 1];
      const int dy = currentBuffer[index] - currentBuffer[index + width];
      int sampleX = x + (dx >> 3);
      int sampleY = y + (dy >> 3);
      sampleX = std::clamp(sampleX, 0, width - 1);
      sampleY = std::clamp(sampleY, 0, height - 1);
      const std::size_t sampleIndex = static_cast<std::size_t>(sampleY) * static_cast<std::size_t>(width) +
                                      static_cast<std::size_t>(sampleX);
      const std::size_t sampleOffset = sampleIndex * kChannels;
      std::memcpy(destination + pixelOffset, source + sampleOffset, kChannels);
    }
  }

  const int nextPage = page_ ^ 1;
  calcWater(nextPage);
  page_ = nextPage;

  return true;
}

}  // namespace avs::effects::trans

