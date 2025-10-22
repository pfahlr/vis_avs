#include "effects/trans/effect_water_bump.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>

namespace {
constexpr std::size_t kChannels = 4u;

bool hasFramebuffer(const avs::core::RenderContext& context, std::size_t requiredBytes) {
  return context.framebuffer.data != nullptr && context.framebuffer.size >= requiredBytes &&
         context.width > 0 && context.height > 0;
}

int randomInRange(avs::core::RenderContext& context, int minValue, int maxValue) {
  if (maxValue <= minValue) {
    return minValue;
  }
  const std::uint32_t value = context.rng.nextUint32();
  const std::uint32_t span = static_cast<std::uint32_t>(maxValue - minValue + 1);
  return minValue + static_cast<int>(value % span);
}

}  // namespace

namespace avs::effects::trans {

bool WaterBump::render(avs::core::RenderContext& context) {
  if (!enabled_) {
    return true;
  }

  const int width = context.width;
  const int height = context.height;
  if (width <= 0 || height <= 0) {
    return true;
  }

  const std::size_t pixelCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
  const std::size_t requiredBytes = pixelCount * kChannels;
  if (!hasFramebuffer(context, requiredBytes)) {
    return true;
  }

  if (width != width_ || height != height_ || heightBuffers_[0].size() != pixelCount) {
    width_ = width;
    height_ = height;
    resetBuffers(pixelCount);
  }

  if (sourceBuffer_.size() != requiredBytes) {
    sourceBuffer_.resize(requiredBytes);
  }
  std::memcpy(sourceBuffer_.data(), context.framebuffer.data, requiredBytes);
  std::memcpy(context.framebuffer.data, sourceBuffer_.data(), requiredBytes);

  if (context.audioBeat) {
    const int maxDim = std::max(width_, height_);
    int radiusPixels = (dropRadiusPercent_ * maxDim) / 100;
    radiusPixels = std::max(radiusPixels, 1);

    int dropX = -1;
    int dropY = -1;
    if (!randomDrop_) {
      switch (dropPositionX_) {
        case 0:
          dropX = width_ / 4;
          break;
        case 2:
          dropX = (width_ * 3) / 4;
          break;
        default:
          dropX = width_ / 2;
          break;
      }
      switch (dropPositionY_) {
        case 0:
          dropY = height_ / 4;
          break;
        case 2:
          dropY = (height_ * 3) / 4;
          break;
        default:
          dropY = height_ / 2;
          break;
      }
    }

    applySineBlob(context, dropX, dropY, radiusPixels, -depth_);
  }

  const auto& current = heightBuffers_[activePage_];
  std::uint8_t* dst = context.framebuffer.data;
  const std::uint8_t* src = sourceBuffer_.data();

  if (width_ < 2 || height_ < 2) {
    return true;
  }

  for (int y = 1; y < height_ - 1; ++y) {
    for (int x = 1; x < width_ - 1; ++x) {
      const std::size_t index = static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) +
                                static_cast<std::size_t>(x);
      const std::size_t pixelOffset = index * kChannels;

      const int dx = current[index] - current[index + 1];
      const int dy = current[index] - current[index + width_];

      int targetX = x + (dx >> 3);
      int targetY = y + (dy >> 3);
      targetX = clampToRange(targetX, 0, width_ - 1);
      targetY = clampToRange(targetY, 0, height_ - 1);

      const std::size_t sampleIndex =
          (static_cast<std::size_t>(targetY) * static_cast<std::size_t>(width_) +
           static_cast<std::size_t>(targetX)) *
          kChannels;

      dst[pixelOffset + 0] = src[sampleIndex + 0];
      dst[pixelOffset + 1] = src[sampleIndex + 1];
      dst[pixelOffset + 2] = src[sampleIndex + 2];
      dst[pixelOffset + 3] = src[sampleIndex + 3];
    }
  }

  calcWater(1 - activePage_);
  activePage_ = 1 - activePage_;

  return true;
}

void WaterBump::setParams(const avs::core::ParamBlock& params) {
  auto readBool = [&params](const std::string& key, bool current) {
    bool value = params.getBool(key, current);
    value = params.getInt(key, value ? 1 : 0) != 0;
    return value;
  };

  enabled_ = readBool("enabled", enabled_);
  randomDrop_ = readBool("random_drop", randomDrop_);

  density_ = clampToRange(params.getInt("density", density_), 0, 10);
  depth_ = clampToRange(params.getInt("depth", depth_), 0, 4000);
  dropPositionX_ = clampToRange(params.getInt("drop_position_x", dropPositionX_), 0, 2);
  dropPositionY_ = clampToRange(params.getInt("drop_position_y", dropPositionY_), 0, 2);
  dropRadiusPercent_ = clampToRange(params.getInt("drop_radius", dropRadiusPercent_), 1, 100);
}

void WaterBump::resetBuffers(std::size_t pixelCount) {
  for (auto& buffer : heightBuffers_) {
    buffer.assign(pixelCount, 0);
  }
  activePage_ = 0;
}

void WaterBump::applySineBlob(avs::core::RenderContext& context, int x, int y, int radius,
                              int height) {
  if (radius <= 0 || heightBuffers_[activePage_].empty()) {
    return;
  }

  const int maxRadius = std::max(1, std::min(width_, height_) - 2);
  radius = clampToRange(radius, 1, maxRadius);

  if (x < 0 || x >= width_) {
    const int margin = radius + 1;
    const int minX = clampToRange(margin, 1, width_ - 2);
    const int maxX = clampToRange(width_ - margin - 1, minX, width_ - 2);
    x = randomInRange(context, minX, maxX);
  }
  if (y < 0 || y >= height_) {
    const int margin = radius + 1;
    const int minY = clampToRange(margin, 1, height_ - 2);
    const int maxY = clampToRange(height_ - margin - 1, minY, height_ - 2);
    y = randomInRange(context, minY, maxY);
  }

  auto& buffer = heightBuffers_[activePage_];

  const double scale = 1024.0 / static_cast<double>(radius);
  const double length = scale * scale;

  const int left = -radius;
  const int right = radius;

  for (int dy = -radius; dy < radius; ++dy) {
    const int row = y + dy;
    if (row <= 0 || row >= height_ - 1) {
      continue;
    }
    for (int dx = left; dx < right; ++dx) {
      const int column = x + dx;
      if (column <= 0 || column >= width_ - 1) {
        continue;
      }
      const int square = dy * dy + dx * dx;
      if (square >= radius * radius) {
        continue;
      }
      const double dist = std::sqrt(static_cast<double>(square) * length);
      const double contribution = (std::cos(dist) + 65535.0) * static_cast<double>(height);
      const int delta = static_cast<int>(contribution / 524288.0);
      const std::size_t index = static_cast<std::size_t>(row) * static_cast<std::size_t>(width_) +
                                static_cast<std::size_t>(column);
      buffer[index] += delta;
    }
  }
}

void WaterBump::calcWater(int nextPage) {
  if (width_ <= 2 || height_ <= 2) {
    return;
  }

  auto& dest = heightBuffers_[nextPage];
  auto& src = heightBuffers_[1 - nextPage];
  const int damping = std::max(0, density_);

  for (int y = 1; y < height_ - 1; ++y) {
    for (int x = 1; x < width_ - 1; ++x) {
      const std::size_t index = static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) +
                                static_cast<std::size_t>(x);
      const int sum = src[index + width_] + src[index - width_] + src[index + 1] + src[index - 1] +
                      src[index - width_ - 1] + src[index - width_ + 1] + src[index + width_ - 1] +
                      src[index + width_ + 1];
      const int newHeight = (sum >> 2) - dest[index];
      dest[index] = newHeight - (newHeight >> damping);
    }
  }
}

int WaterBump::clampToRange(int value, int minValue, int maxValue) {
  if (value < minValue) {
    return minValue;
  }
  if (value > maxValue) {
    return maxValue;
  }
  return value;
}

}  // namespace avs::effects::trans
