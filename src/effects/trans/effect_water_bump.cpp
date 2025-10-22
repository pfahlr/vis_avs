#include "effects/trans/effect_water_bump.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace avs::effects::trans {
namespace {
constexpr float kLightDirX = -0.35f;
constexpr float kLightDirY = -0.25f;
constexpr float kLightDirZ = 0.9f;
constexpr float kNormalScale = 1.0f / 256.0f;
constexpr int kDisplacementShift = 3;

float normalizeAndDot(float nx, float ny, float nz) {
  const float length = std::sqrt(nx * nx + ny * ny + nz * nz);
  if (length <= 0.00001f) {
    return 1.0f;
  }

  const float invLen = 1.0f / length;
  nx *= invLen;
  ny *= invLen;
  nz *= invLen;

  const float lightLength =
      std::sqrt(kLightDirX * kLightDirX + kLightDirY * kLightDirY + kLightDirZ * kLightDirZ);
  const float lx = kLightDirX / lightLength;
  const float ly = kLightDirY / lightLength;
  const float lz = kLightDirZ / lightLength;

  return nx * lx + ny * ly + nz * lz;
}

std::uint8_t applyShading(std::uint8_t value, float shading) {
  const float shaded = std::clamp(static_cast<float>(value) * shading, 0.0f, 255.0f);
  return static_cast<std::uint8_t>(std::lround(shaded));
}

int clampDensity(int density) { return std::clamp(density, 1, 10); }

}  // namespace

void WaterBump::setParams(const avs::core::ParamBlock& params) {
  if (params.contains("enabled")) {
    enabled_ = params.getBool("enabled", enabled_);
  }
  if (params.contains("density")) {
    densityShift_ = clampDensity(params.getInt("density", densityShift_));
  }
  if (params.contains("depth")) {
    dropDepth_ = std::max(0, params.getInt("depth", dropDepth_));
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
    dropRadiusPercent_ = std::clamp(params.getInt("drop_radius", dropRadiusPercent_), 1, 200);
  }
}

bool WaterBump::render(avs::core::RenderContext& context) {
  if (!enabled_) {
    return true;
  }

  if (context.width <= 0 || context.height <= 0) {
    return true;
  }

  ensureBuffers(context.width, context.height);
  if (width_ <= 0 || height_ <= 0) {
    return true;
  }

  const std::size_t pixelCount =
      static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_);
  const std::size_t requiredBytes = pixelCount * kChannels;
  if (context.framebuffer.data == nullptr || context.framebuffer.size < requiredBytes) {
    return true;
  }

  if (scratch_.size() < requiredBytes) {
    scratch_.resize(requiredBytes);
  }

  if (context.audioBeat && dropDepth_ > 0) {
    const int maxDimension = std::max(width_, height_);
    int radiusPixels = (dropRadiusPercent_ * maxDimension) / 100;
    radiusPixels = std::max(radiusPixels, 1);
    const int maxRadius = std::max(1, std::min((width_ - 2) / 2, (height_ - 2) / 2));
    radiusPixels = std::clamp(radiusPixels, 1, std::max(1, maxRadius));

    const auto choosePosition = [&](int preset, int size) {
      switch (preset) {
        case 0:
          return size / 4;
        case 2:
          return (size * 3) / 4;
        default:
          return size / 2;
      }
    };

    const auto randomInRange = [&](int minValue, int maxValue) {
      if (maxValue <= minValue) {
        return minValue;
      }
      const std::uint32_t raw = context.rng.nextUint32();
      const std::uint32_t span = static_cast<std::uint32_t>(maxValue - minValue + 1);
      return minValue + static_cast<int>(raw % span);
    };

    const int minX = std::max(radiusPixels, 1);
    const int maxX = std::max(minX, width_ - radiusPixels - 1);
    const int minY = std::max(radiusPixels, 1);
    const int maxY = std::max(minY, height_ - radiusPixels - 1);

    int dropX = choosePosition(dropPositionX_, width_);
    int dropY = choosePosition(dropPositionY_, height_);

    if (randomDrop_) {
      dropX = randomInRange(minX, maxX);
      dropY = randomInRange(minY, maxY);
    }

    dropX = std::clamp(dropX, minX, maxX);
    dropY = std::clamp(dropY, minY, maxY);

    sineBlob(dropX, dropY, radiusPixels, -dropDepth_);
  }

  applyDisplacementAndLighting(context.framebuffer.data, scratch_.data());

  calcWater(1 - activePage_);
  activePage_ = 1 - activePage_;

  std::memcpy(context.framebuffer.data, scratch_.data(), requiredBytes);

  return true;
}

void WaterBump::ensureBuffers(int width, int height) {
  if (width <= 0 || height <= 0) {
    width_ = 0;
    height_ = 0;
    activePage_ = 0;
    scratch_.clear();
    for (auto& buffer : heightBuffers_) {
      buffer.clear();
    }
    return;
  }

  const std::size_t pixelCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
  const bool sizeChanged = width != width_ || height != height_ ||
                           heightBuffers_[0].size() != pixelCount ||
                           heightBuffers_[1].size() != pixelCount;
  if (sizeChanged) {
    width_ = width;
    height_ = height;
    activePage_ = 0;
    for (auto& buffer : heightBuffers_) {
      buffer.assign(pixelCount, 0);
    }
    scratch_.assign(pixelCount * kChannels, 0u);
  }
}

void WaterBump::sineBlob(int x, int y, int radius, int height) {
  if (radius <= 0 || width_ <= 0 || height_ <= 0) {
    return;
  }

  radius = std::min(radius, std::max(1, std::min(width_, height_) - 1));

  auto& buffer = heightBuffers_[activePage_];
  const int w = width_;
  const int h = height_;

  if (x < radius) {
    x = radius;
  } else if (x >= w - radius) {
    x = w - radius - 1;
  }
  if (y < radius) {
    y = radius;
  } else if (y >= h - radius) {
    y = h - radius - 1;
  }

  const int left = -radius;
  const int right = radius;
  const int top = -radius;
  const int bottom = radius;
  const int radiusSquare = radius * radius;
  const double invRadius = radius > 0 ? (1024.0 / static_cast<double>(radius)) : 0.0;
  const double length = invRadius * invRadius;

  for (int offsetY = top; offsetY < bottom; ++offsetY) {
    const int py = y + offsetY;
    if (py <= 0 || py >= h - 1) {
      continue;
    }
    for (int offsetX = left; offsetX < right; ++offsetX) {
      const int px = x + offsetX;
      if (px <= 0 || px >= w - 1) {
        continue;
      }
      const int square = offsetX * offsetX + offsetY * offsetY;
      if (square >= radiusSquare) {
        continue;
      }

      const double dist = std::sqrt(static_cast<double>(square) * length);
      const double weight = std::cos(dist) + 65535.0;
      const int delta = static_cast<int>((weight * static_cast<double>(height)) / 524288.0);
      const std::size_t index =
          static_cast<std::size_t>(py) * static_cast<std::size_t>(w) + static_cast<std::size_t>(px);
      buffer[index] += delta;
    }
  }
}

void WaterBump::calcWater(int destinationPage) {
  if (width_ <= 0 || height_ <= 0) {
    return;
  }

  const std::size_t pixelCount =
      static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_);
  if (heightBuffers_[destinationPage].size() != pixelCount) {
    heightBuffers_[destinationPage].assign(pixelCount, 0);
  }

  if (width_ < 3 || height_ < 3) {
    heightBuffers_[destinationPage] = heightBuffers_[activePage_];
    return;
  }

  const auto& src = heightBuffers_[activePage_];
  auto& dst = heightBuffers_[destinationPage];
  const int damping = clampDensity(densityShift_);
  const int w = width_;
  const int h = height_;

  for (int y = 1; y < h - 1; ++y) {
    const std::size_t row = static_cast<std::size_t>(y) * static_cast<std::size_t>(w);
    for (int x = 1; x < w - 1; ++x) {
      const std::size_t idx = row + static_cast<std::size_t>(x);
      const int accum = src[idx - w - 1] + src[idx - w] + src[idx - w + 1] + src[idx - 1] +
                        src[idx + 1] + src[idx + w - 1] + src[idx + w] + src[idx + w + 1];
      const int newHeight = (accum >> 2) - dst[idx];
      dst[idx] = newHeight - (newHeight >> damping);
    }
  }

  // Clear the borders to keep the simulation stable.
  for (int x = 0; x < w; ++x) {
    dst[x] = 0;
    dst[(h - 1) * w + x] = 0;
  }
  for (int y = 0; y < h; ++y) {
    dst[y * w] = 0;
    dst[y * w + (w - 1)] = 0;
  }
}

void WaterBump::applyDisplacementAndLighting(const std::uint8_t* src, std::uint8_t* dst) const {
  if (src == nullptr || dst == nullptr || width_ <= 0 || height_ <= 0) {
    return;
  }

  const auto& heightMap = heightBuffers_[activePage_];
  const int w = width_;
  const int h = height_;
  const std::size_t widthSize = static_cast<std::size_t>(w);

  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      const std::size_t idx = static_cast<std::size_t>(y) * widthSize + static_cast<std::size_t>(x);
      const std::size_t pixelOffset = idx * kChannels;

      if (x == 0 || y == 0 || x == w - 1 || y == h - 1) {
        std::memcpy(dst + pixelOffset, src + pixelOffset, kChannels);
        continue;
      }

      const std::int32_t baseHeight = heightMap[idx];
      const std::int32_t dxHeight = baseHeight - heightMap[idx + 1];
      const std::int32_t dyHeight = baseHeight - heightMap[idx + static_cast<std::size_t>(w)];

      int sampleX = x + (dxHeight >> kDisplacementShift);
      int sampleY = y + (dyHeight >> kDisplacementShift);
      if (sampleX < 0 || sampleX >= w || sampleY < 0 || sampleY >= h) {
        sampleX = x;
        sampleY = y;
      }

      const std::size_t sampleIdx =
          (static_cast<std::size_t>(sampleY) * widthSize + static_cast<std::size_t>(sampleX)) *
          kChannels;
      const std::uint8_t r = src[sampleIdx + 0];
      const std::uint8_t g = src[sampleIdx + 1];
      const std::uint8_t b = src[sampleIdx + 2];
      const std::uint8_t a = src[sampleIdx + 3];

      const std::int32_t gradX = heightMap[idx + 1] - heightMap[idx - 1];
      const std::int32_t gradY = heightMap[idx + static_cast<std::size_t>(w)] -
                                 heightMap[idx - static_cast<std::size_t>(w)];

      const float nx = static_cast<float>(gradX) * kNormalScale;
      const float ny = static_cast<float>(gradY) * kNormalScale;
      const float nz = 1.0f;
      const float dot = std::clamp(normalizeAndDot(nx, ny, nz), -1.0f, 1.0f);
      const float shading = std::clamp(0.9f + 0.4f * dot, 0.0f, 2.0f);

      dst[pixelOffset + 0] = applyShading(r, shading);
      dst[pixelOffset + 1] = applyShading(g, shading);
      dst[pixelOffset + 2] = applyShading(b, shading);
      dst[pixelOffset + 3] = a;
    }
  }
}

}  // namespace avs::effects::trans
