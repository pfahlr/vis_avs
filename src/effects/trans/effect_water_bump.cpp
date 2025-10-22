#include "effects/trans/effect_water_bump.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace avs::effects::trans {

namespace {

int clampInt(int value, int minValue, int maxValue) {
  if (value < minValue) {
    return minValue;
  }
  if (value > maxValue) {
    return maxValue;
  }
  return value;
}

}  // namespace

bool WaterBump::hasFramebuffer(const avs::core::RenderContext& context,
                               std::size_t requiredBytes) const {
  return context.framebuffer.data != nullptr && context.framebuffer.size >= requiredBytes &&
         context.width > 0 && context.height > 0;
}

void WaterBump::ensureBuffers(int width, int height) {
  if (width <= 0 || height <= 0) {
    bufferWidth_ = 0;
    bufferHeight_ = 0;
    for (auto& buffer : heightBuffers_) {
      buffer.clear();
    }
    page_ = 0;
    return;
  }

  if (width != bufferWidth_ || height != bufferHeight_) {
    bufferWidth_ = width;
    bufferHeight_ = height;
    const std::size_t pixelCount =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    for (auto& buffer : heightBuffers_) {
      buffer.assign(pixelCount, 0);
    }
    page_ = 0;
  } else {
    const std::size_t pixelCount =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    for (auto& buffer : heightBuffers_) {
      if (buffer.size() != pixelCount) {
        buffer.assign(pixelCount, 0);
      }
    }
  }
}

void WaterBump::applyBeatDrop(avs::core::RenderContext& context) {
  if (!context.audioBeat) {
    return;
  }
  if (depth_ == 0 || dropRadiusPercent_ <= 0 || bufferWidth_ <= 2 || bufferHeight_ <= 2) {
    return;
  }

  const int maxDimension = std::max(bufferWidth_, bufferHeight_);
  if (maxDimension <= 0) {
    return;
  }

  int radius = dropRadiusPercent_ * maxDimension / 100;
  radius = clampInt(radius, 1, std::max(1, std::min(bufferWidth_, bufferHeight_) / 2));

  if (radius <= 0) {
    return;
  }

  auto randomInRange = [&](int minValue, int maxValue) {
    if (maxValue <= minValue) {
      return minValue;
    }
    const std::uint32_t span = static_cast<std::uint32_t>(maxValue - minValue + 1);
    const std::uint32_t value = context.rng.nextUint32();
    return minValue + static_cast<int>(value % span);
  };

  int x = bufferWidth_ / 2;
  int y = bufferHeight_ / 2;

  if (randomDrop_) {
    const int minX = clampInt(radius + 1, 1, bufferWidth_ - 2);
    const int minY = clampInt(radius + 1, 1, bufferHeight_ - 2);
    const int usableMaxX = std::max(minX, bufferWidth_ - radius - 2);
    const int usableMaxY = std::max(minY, bufferHeight_ - radius - 2);
    x = randomInRange(minX, usableMaxX);
    y = randomInRange(minY, usableMaxY);
  } else {
    switch (dropPositionX_) {
      case 0:
        x = bufferWidth_ / 4;
        break;
      case 2:
        x = (bufferWidth_ * 3) / 4;
        break;
      default:
        x = bufferWidth_ / 2;
        break;
    }

    switch (dropPositionY_) {
      case 0:
        y = bufferHeight_ / 4;
        break;
      case 2:
        y = (bufferHeight_ * 3) / 4;
        break;
      default:
        y = bufferHeight_ / 2;
        break;
    }
  }

  const int amplitude = depth_ > 0 ? -depth_ : depth_;
  sineBlob(x, y, radius, amplitude);
}

void WaterBump::sineBlob(int x, int y, int radius, int height) {
  if (radius <= 0 || height == 0 || bufferWidth_ <= 0 || bufferHeight_ <= 0) {
    return;
  }

  radius = std::min(radius, std::max(1, std::min(bufferWidth_, bufferHeight_) - 2));
  const int minX = std::max(1, x - radius);
  const int maxX = std::min(bufferWidth_ - 2, x + radius);
  const int minY = std::max(1, y - radius);
  const int maxY = std::min(bufferHeight_ - 2, y + radius);

  if (minX >= maxX || minY >= maxY) {
    return;
  }

  const int radiusSquared = radius * radius;
  const double length = (radius != 0) ? (1024.0 / static_cast<double>(radius)) : 0.0;
  const double lengthSquared = length * length;

  std::vector<int>& current = heightBuffers_[page_];

  for (int yy = minY; yy < maxY; ++yy) {
    const int dy = yy - y;
    for (int xx = minX; xx < maxX; ++xx) {
      const int dx = xx - x;
      const int distanceSquared = dx * dx + dy * dy;
      if (distanceSquared < radiusSquared) {
        const double dist = std::sqrt(static_cast<double>(distanceSquared) * lengthSquared);
        const double value = (std::cos(dist) + 65535.0) * static_cast<double>(height) / 524288.0;
        const std::size_t index =
            static_cast<std::size_t>(yy) * static_cast<std::size_t>(bufferWidth_) +
            static_cast<std::size_t>(xx);
        current[index] += static_cast<int>(value);
      }
    }
  }
}

void WaterBump::calcWater(int targetPage) {
  if (bufferWidth_ < 3 || bufferHeight_ < 3) {
    return;
  }

  std::vector<int>& newBuffer = heightBuffers_[targetPage];
  const std::vector<int>& oldBuffer = heightBuffers_[1 - targetPage];

  for (int y = 1; y < bufferHeight_ - 1; ++y) {
    const int rowOffset = y * bufferWidth_;
    for (int x = 1; x < bufferWidth_ - 1; ++x) {
      const int idx = rowOffset + x;

      const int sum = oldBuffer[idx + bufferWidth_] + oldBuffer[idx - bufferWidth_] +
                      oldBuffer[idx + 1] + oldBuffer[idx - 1] + oldBuffer[idx - bufferWidth_ - 1] +
                      oldBuffer[idx - bufferWidth_ + 1] + oldBuffer[idx + bufferWidth_ - 1] +
                      oldBuffer[idx + bufferWidth_ + 1];

      const int newHeight = (sum >> 2) - newBuffer[idx];
      const int dampening = density_ <= 0 ? newHeight : (newHeight >> density_);
      newBuffer[idx] = newHeight - dampening;
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
  const std::size_t requiredBytes =
      static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * kChannels;

  if (!hasFramebuffer(context, requiredBytes)) {
    return true;
  }

  ensureBuffers(width, height);

  if (inputCopy_.size() != requiredBytes) {
    inputCopy_.resize(requiredBytes);
  }
  if (scratch_.size() != requiredBytes) {
    scratch_.resize(requiredBytes);
  }

  std::memcpy(inputCopy_.data(), context.framebuffer.data, requiredBytes);
  std::memcpy(scratch_.data(), context.framebuffer.data, requiredBytes);

  applyBeatDrop(context);

  const std::vector<int>& current = heightBuffers_[page_];
  std::uint8_t* dst = scratch_.data();
  const std::uint8_t* src = inputCopy_.data();

  if (width >= 3 && height >= 3) {
    const int totalPixels = width * height;
    for (int y = 1; y < height - 1; ++y) {
      for (int x = 1; x < width - 1; ++x) {
        const int idx = y * width + x;
        const int dx = current[idx] - current[idx + 1];
        const int dy = current[idx] - current[idx + width];
        const int offset = idx + (dy >> 3) * width + (dx >> 3);
        if (offset >= 0 && offset < totalPixels) {
          const std::size_t dstOffset = static_cast<std::size_t>(idx) * kChannels;
          const std::size_t srcOffset = static_cast<std::size_t>(offset) * kChannels;
          dst[dstOffset + 0] = src[srcOffset + 0];
          dst[dstOffset + 1] = src[srcOffset + 1];
          dst[dstOffset + 2] = src[srcOffset + 2];
          dst[dstOffset + 3] = src[srcOffset + 3];
        }
      }
    }
  }

  const int nextPage = 1 - page_;
  calcWater(nextPage);
  page_ = nextPage;

  std::memcpy(context.framebuffer.data, scratch_.data(), requiredBytes);
  return true;
}

void WaterBump::setParams(const avs::core::ParamBlock& params) {
  if (params.contains("enabled")) {
    enabled_ = params.getBool("enabled", enabled_);
  }
  density_ = clampInt(params.getInt("density", density_), 0, 10);
  depth_ = std::max(0, params.getInt("depth", depth_));
  randomDrop_ = params.getBool("random_drop", randomDrop_);
  dropPositionX_ = clampInt(params.getInt("drop_position_x", dropPositionX_), 0, 2);
  dropPositionY_ = clampInt(params.getInt("drop_position_y", dropPositionY_), 0, 2);
  dropRadiusPercent_ = clampInt(params.getInt("drop_radius", dropRadiusPercent_), 0, 100);
  method_ = params.getInt("method", method_);
}

}  // namespace avs::effects::trans
