#include <avs/effects/trans/effect_water_bump.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

#include <avs/core/DeterministicRng.hpp>
#include <avs/core/RenderContext.hpp>

namespace {

bool hasFramebuffer(const avs::core::RenderContext& context, std::size_t requiredBytes) {
  return context.framebuffer.data != nullptr && context.framebuffer.size >= requiredBytes &&
         context.width > 0 && context.height > 0;
}

int clampInt(int value, int minValue, int maxValue) {
  return std::max(minValue, std::min(maxValue, value));
}

bool readBoolLike(const avs::core::ParamBlock& params, const std::string& key, bool fallback) {
  if (!params.contains(key)) {
    return fallback;
  }
  bool value = params.getBool(key, fallback);
  value = params.getInt(key, value ? 1 : 0) != 0;
  return value;
}

int randomInRange(avs::core::DeterministicRng& rng, int minValue, int maxValue) {
  if (maxValue <= minValue) {
    return minValue;
  }
  const std::uint32_t span = static_cast<std::uint32_t>(maxValue - minValue + 1);
  const std::uint32_t value = rng.nextUint32();
  return minValue + static_cast<int>(value % span);
}

}  // namespace

namespace avs::effects::trans {

void WaterBump::setParams(const avs::core::ParamBlock& params) {
  if (params.contains("enabled")) {
    enabled_ = params.getBool("enabled", enabled_);
  }
  if (params.contains("density")) {
    density_ = clampInt(params.getInt("density", density_), 0, 10);
  }
  if (params.contains("depth")) {
    depth_ = std::max(0, params.getInt("depth", depth_));
  }
  if (params.contains("random_drop")) {
    randomDrop_ = readBoolLike(params, "random_drop", randomDrop_);
  }
  if (params.contains("drop_position_x")) {
    dropPositionX_ = clampInt(params.getInt("drop_position_x", dropPositionX_), 0, 2);
  }
  if (params.contains("drop_position_y")) {
    dropPositionY_ = clampInt(params.getInt("drop_position_y", dropPositionY_), 0, 2);
  }
  if (params.contains("drop_radius")) {
    dropRadius_ = std::max(1, params.getInt("drop_radius", dropRadius_));
  }
  if (params.contains("method")) {
    method_ = clampInt(params.getInt("method", method_), 0, 1);
  }
}

bool WaterBump::ensureResources(int width, int height, std::size_t requiredBytes) {
  if (width <= 0 || height <= 0) {
    return false;
  }

  const std::size_t totalPixels =
      static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
  if (bufferWidth_ != width || bufferHeight_ != height || heightBuffers_[0].size() != totalPixels ||
      heightBuffers_[1].size() != totalPixels) {
    bufferWidth_ = width;
    bufferHeight_ = height;
    currentPage_ = 0;
    heightBuffers_[0].assign(totalPixels, 0);
    heightBuffers_[1].assign(totalPixels, 0);
  }

  if (scratch_.size() < requiredBytes) {
    scratch_.resize(requiredBytes);
  }

  return true;
}

void WaterBump::applyDrop(avs::core::RenderContext& context) {
  if (!context.audioBeat || depth_ <= 0) {
    return;
  }
  if (bufferWidth_ <= 2 || bufferHeight_ <= 2) {
    return;
  }

  int radiusPixels = std::max(1, dropRadius_);
  int centerX = bufferWidth_ / 2;
  int centerY = bufferHeight_ / 2;

  if (randomDrop_) {
    const int maxDimension = std::max(bufferWidth_, bufferHeight_);
    radiusPixels = std::max(1, (dropRadius_ * maxDimension) / 100);
    const int minX = std::max(1, radiusPixels + 1);
    const int maxX = std::max(minX, bufferWidth_ - radiusPixels - 2);
    const int minY = std::max(1, radiusPixels + 1);
    const int maxY = std::max(minY, bufferHeight_ - radiusPixels - 2);
    centerX = clampInt(randomInRange(context.rng, minX, maxX), 1, bufferWidth_ - 2);
    centerY = clampInt(randomInRange(context.rng, minY, maxY), 1, bufferHeight_ - 2);
  } else {
    switch (dropPositionX_) {
      case 0:
        centerX = bufferWidth_ / 4;
        break;
      case 1:
        centerX = bufferWidth_ / 2;
        break;
      case 2:
        centerX = (bufferWidth_ * 3) / 4;
        break;
      default:
        break;
    }
    switch (dropPositionY_) {
      case 0:
        centerY = bufferHeight_ / 4;
        break;
      case 1:
        centerY = bufferHeight_ / 2;
        break;
      case 2:
        centerY = (bufferHeight_ * 3) / 4;
        break;
      default:
        break;
    }
    centerX = clampInt(centerX, 1, std::max(1, bufferWidth_ - 2));
    centerY = clampInt(centerY, 1, std::max(1, bufferHeight_ - 2));
  }

  if (method_ == 1) {
    applyHeightBlob(centerX, centerY, radiusPixels, -depth_, context.rng);
  } else {
    applySineBlob(centerX, centerY, radiusPixels, -depth_, context.rng);
  }
}

void WaterBump::applySineBlob(int x, int y, int radius, int heightDelta,
                              avs::core::DeterministicRng& rng) {
  if (radius <= 0 || bufferWidth_ <= 0 || bufferHeight_ <= 0) {
    return;
  }

  const int width = bufferWidth_;
  const int height = bufferHeight_;
  radius = clampInt(radius, 1, std::max(1, std::min(width, height) - 1));

  auto& heightMap = heightBuffers_[currentPage_];
  if (heightMap.empty()) {
    return;
  }

  if (x < 0 || x >= width) {
    const int minX = std::max(1, radius + 1);
    const int maxX = std::max(minX, width - radius - 2);
    x = clampInt(randomInRange(rng, minX, maxX), 1, width - 2);
  }
  if (y < 0 || y >= height) {
    const int minY = std::max(1, radius + 1);
    const int maxY = std::max(minY, height - radius - 2);
    y = clampInt(randomInRange(rng, minY, maxY), 1, height - 2);
  }

  int left = -radius;
  int right = radius;
  int top = -radius;
  int bottom = radius;

  if (x - radius < 1) {
    left -= (x - radius - 1);
  }
  if (y - radius < 1) {
    top -= (y - radius - 1);
  }
  if (x + radius > width - 1) {
    right -= (x + radius - width + 1);
  }
  if (y + radius > height - 1) {
    bottom -= (y + radius - height + 1);
  }

  const int radiusSquared = radius * radius;
  const double scale = 1024.0 / static_cast<double>(radius);
  const double length = scale * scale;

  for (int cy = top; cy < bottom; ++cy) {
    const int actualY = y + cy;
    if (actualY <= 0 || actualY >= height - 1) {
      continue;
    }
    for (int cx = left; cx < right; ++cx) {
      const int actualX = x + cx;
      if (actualX <= 0 || actualX >= width - 1) {
        continue;
      }
      const int square = cy * cy + cx * cx;
      if (square >= radiusSquared) {
        continue;
      }
      const double dist = std::sqrt(static_cast<double>(square) * length);
      const double scaled = (std::cos(dist) + 65535.0) * static_cast<double>(heightDelta);
      const int delta = static_cast<int>(scaled) >> 19;
      if (delta == 0) {
        continue;
      }
      const std::size_t index =
          static_cast<std::size_t>(actualY) * static_cast<std::size_t>(width) +
          static_cast<std::size_t>(actualX);
      heightMap[index] += delta;
    }
  }
}

void WaterBump::applyHeightBlob(int x, int y, int radius, int heightDelta,
                                avs::core::DeterministicRng& rng) {
  if (radius <= 0 || bufferWidth_ <= 0 || bufferHeight_ <= 0) {
    return;
  }

  const int width = bufferWidth_;
  const int height = bufferHeight_;
  radius = clampInt(radius, 1, std::max(1, std::min(width, height) - 1));

  auto& heightMap = heightBuffers_[currentPage_];
  if (heightMap.empty()) {
    return;
  }

  if (x < 0 || x >= width) {
    const int minX = std::max(1, radius + 1);
    const int maxX = std::max(minX, width - radius - 2);
    x = clampInt(randomInRange(rng, minX, maxX), 1, width - 2);
  }
  if (y < 0 || y >= height) {
    const int minY = std::max(1, radius + 1);
    const int maxY = std::max(minY, height - radius - 2);
    y = clampInt(randomInRange(rng, minY, maxY), 1, height - 2);
  }

  int left = -radius;
  int right = radius;
  int top = -radius;
  int bottom = radius;

  if (x - radius < 1) {
    left -= (x - radius - 1);
  }
  if (y - radius < 1) {
    top -= (y - radius - 1);
  }
  if (x + radius > width - 1) {
    right -= (x + radius - width + 1);
  }
  if (y + radius > height - 1) {
    bottom -= (y + radius - height + 1);
  }

  const int radiusSquared = radius * radius;
  for (int cy = top; cy < bottom; ++cy) {
    const int actualY = y + cy;
    if (actualY <= 0 || actualY >= height - 1) {
      continue;
    }
    for (int cx = left; cx < right; ++cx) {
      const int actualX = x + cx;
      if (actualX <= 0 || actualX >= width - 1) {
        continue;
      }
      if (cx * cx + cy * cy >= radiusSquared) {
        continue;
      }
      const std::size_t index =
          static_cast<std::size_t>(actualY) * static_cast<std::size_t>(width) +
          static_cast<std::size_t>(actualX);
      heightMap[index] += heightDelta;
    }
  }
}

void WaterBump::simulateWater() {
  if (bufferWidth_ <= 0 || bufferHeight_ <= 0) {
    return;
  }

  const int nextPage = 1 - currentPage_;
  auto& current = heightBuffers_[currentPage_];
  auto& next = heightBuffers_[nextPage];
  if (current.empty() || next.empty()) {
    return;
  }

  const int width = bufferWidth_;
  const int height = bufferHeight_;

  if (width < 3 || height < 3) {
    std::fill(next.begin(), next.end(), 0);
    currentPage_ = nextPage;
    return;
  }

  const int dampingShift = clampInt(density_, 0, 10);

  for (int y = 1; y < height - 1; ++y) {
    const int rowOffset = y * width;
    for (int x = 1; x < width - 1; ++x) {
      const std::size_t index = static_cast<std::size_t>(rowOffset + x);
      int newHeight = current[index - width] + current[index + width] + current[index - 1] +
                      current[index + 1] + current[index - width - 1] + current[index - width + 1] +
                      current[index + width - 1] + current[index + width + 1];
      newHeight >>= 2;
      newHeight -= next[index];
      next[index] = newHeight - (newHeight >> dampingShift);
    }
  }

  for (int x = 0; x < width; ++x) {
    next[static_cast<std::size_t>(x)] = 0;
    next[static_cast<std::size_t>((height - 1) * width + x)] = 0;
  }
  for (int y = 0; y < height; ++y) {
    const std::size_t base = static_cast<std::size_t>(y) * static_cast<std::size_t>(width);
    next[base] = 0;
    next[base + static_cast<std::size_t>(width - 1)] = 0;
  }

  currentPage_ = nextPage;
}

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

  if (!ensureResources(context.width, context.height, requiredBytes)) {
    return true;
  }

  applyDrop(context);

  const auto& heightMap = heightBuffers_[currentPage_];
  if (heightMap.empty()) {
    return true;
  }

  std::uint8_t* dst = scratch_.data();
  const std::uint8_t* src = context.framebuffer.data;
  const int widthInt = context.width;
  const int heightInt = context.height;

  for (int y = 0; y < heightInt; ++y) {
    for (int x = 0; x < widthInt; ++x) {
      const std::size_t index = static_cast<std::size_t>(y) * width + static_cast<std::size_t>(x);
      const std::size_t pixelOffset = index * kChannels;
      const int centerHeight = heightMap[index];
      const int rightHeight = (x + 1 < widthInt) ? heightMap[index + 1] : centerHeight;
      const int downHeight = (y + 1 < heightInt) ? heightMap[index + width] : centerHeight;
      const int dx = centerHeight - rightHeight;
      const int dy = centerHeight - downHeight;
      const int sampleX = x + (dx >> 3);
      const int sampleY = y + (dy >> 3);
      if (sampleX < 0 || sampleX >= widthInt || sampleY < 0 || sampleY >= heightInt) {
        dst[pixelOffset + 0] = src[pixelOffset + 0];
        dst[pixelOffset + 1] = src[pixelOffset + 1];
        dst[pixelOffset + 2] = src[pixelOffset + 2];
        dst[pixelOffset + 3] = src[pixelOffset + 3];
        continue;
      }
      const std::size_t sampleIndex =
          (static_cast<std::size_t>(sampleY) * width + static_cast<std::size_t>(sampleX)) *
          kChannels;
      dst[pixelOffset + 0] = src[sampleIndex + 0];
      dst[pixelOffset + 1] = src[sampleIndex + 1];
      dst[pixelOffset + 2] = src[sampleIndex + 2];
      dst[pixelOffset + 3] = src[sampleIndex + 3];
    }
  }

  std::memcpy(context.framebuffer.data, dst, requiredBytes);

  simulateWater();

  return true;
}

}  // namespace avs::effects::trans
