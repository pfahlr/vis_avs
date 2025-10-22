#include "effects/trans/effect_water_bump.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace avs::effects::trans {

namespace {
constexpr float kPi = 3.14159265358979323846f;
constexpr std::array<float, 3> kLightDirection = {-0.398f, -0.597f, 0.696f};
constexpr float kAmbient = 0.35f;
constexpr float kDiffuse = 0.65f;
constexpr float kNormalScale = 256.0f;

bool hasValidFramebuffer(const avs::core::RenderContext& context, std::size_t requiredBytes) {
  return context.framebuffer.data != nullptr && context.framebuffer.size >= requiredBytes &&
         context.width > 0 && context.height > 0;
}

int clampToRange(int value, int minValue, int maxValue) {
  return std::max(minValue, std::min(value, maxValue));
}

std::uint32_t nextUint32(avs::core::DeterministicRng& rng) { return rng.nextUint32(); }

}  // namespace

void WaterBump::setParams(const avs::core::ParamBlock& params) {
  if (params.contains("enabled")) {
    enabled_ = params.getBool("enabled", enabled_);
  }
  if (params.contains("density")) {
    density_ = clampToRange(params.getInt("density", density_), 0, 10);
  }
  if (params.contains("depth")) {
    depth_ = clampToRange(params.getInt("depth", depth_), 0, 4000);
  }
  if (params.contains("random_drop")) {
    randomDrop_ = params.getBool("random_drop", randomDrop_);
  }
  if (params.contains("drop_position_x")) {
    dropPosX_ = clampToRange(params.getInt("drop_position_x", dropPosX_), 0, 2);
  }
  if (params.contains("drop_position_y")) {
    dropPosY_ = clampToRange(params.getInt("drop_position_y", dropPosY_), 0, 2);
  }
  if (params.contains("drop_radius")) {
    dropRadiusPercent_ = clampToRange(params.getInt("drop_radius", dropRadiusPercent_), 1, 100);
  }
  if (params.contains("method")) {
    method_ = clampToRange(params.getInt("method", method_), 0, 1);
  }
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
  if (!hasValidFramebuffer(context, requiredBytes)) {
    return true;
  }

  ensureBuffers(width, height);

  if (context.audioBeat) {
    spawnDrop(context);
  }

  const std::uint8_t* srcPixels = context.framebuffer.data;
  renderFrame(srcPixels);

  const int nextPage = 1 - currentPage_;
  updateWater(nextPage);
  currentPage_ = nextPage;

  std::copy_n(scratch_.data(), requiredBytes, context.framebuffer.data);
  return true;
}

void WaterBump::ensureBuffers(std::size_t width, std::size_t height) {
  if (width_ == static_cast<int>(width) && height_ == static_cast<int>(height)) {
    return;
  }

  width_ = static_cast<int>(width);
  height_ = static_cast<int>(height);
  const std::size_t totalPixels = width * height;
  resetBuffers(totalPixels);
}

void WaterBump::resetBuffers(std::size_t totalPixels) {
  for (auto& buffer : heightBuffers_) {
    buffer.assign(totalPixels, 0);
  }
  scratch_.assign(totalPixels * kChannels, 0u);
  currentPage_ = 0;
}

void WaterBump::spawnDrop(avs::core::RenderContext& context) {
  if (width_ < 3 || height_ < 3) {
    return;
  }

  const int maxDimension = std::max(width_, height_);
  const int minDimension = std::min(width_, height_);
  int radius = (dropRadiusPercent_ * maxDimension) / 100;
  const int maxRadius = std::max(1, minDimension / 2 - 2);
  radius = clampToRange(radius, 1, std::max(1, maxRadius));
  if (radius <= 0) {
    return;
  }

  const auto clampCenter = [radius](int value, int limit) {
    const int minValue = radius + 1;
    const int maxValue = std::max(minValue, limit - radius - 2);
    return clampToRange(value, minValue, maxValue);
  };

  int centerX = width_ / 2;
  int centerY = height_ / 2;

  if (randomDrop_) {
    const int minX = radius + 1;
    const int maxX = std::max(minX, width_ - radius - 2);
    const int minY = radius + 1;
    const int maxY = std::max(minY, height_ - radius - 2);
    if (maxX > minX) {
      centerX = minX + static_cast<int>(nextUint32(context.rng) %
                                        static_cast<std::uint32_t>(maxX - minX + 1));
    } else {
      centerX = minX;
    }
    if (maxY > minY) {
      centerY = minY + static_cast<int>(nextUint32(context.rng) %
                                        static_cast<std::uint32_t>(maxY - minY + 1));
    } else {
      centerY = minY;
    }
  } else {
    const auto mapPosition = [](int selector, int size) {
      switch (selector) {
        case 0:
          return size / 4;
        case 2:
          return (size * 3) / 4;
        default:
          return size / 2;
      }
    };
    centerX = mapPosition(dropPosX_, width_);
    centerY = mapPosition(dropPosY_, height_);
    centerX = clampCenter(centerX, width_);
    centerY = clampCenter(centerY, height_);
  }

  const int heightDelta = -std::abs(depth_);
  if (method_ == 1) {
    applyHeightBlob(centerX, centerY, radius, heightDelta);
  } else {
    applySineBlob(centerX, centerY, radius, heightDelta);
  }
}

void WaterBump::applySineBlob(int centerX, int centerY, int radius, int heightDelta) {
  if (radius <= 0) {
    return;
  }

  auto& buffer = heightBuffers_[currentPage_];
  const int radiusSq = radius * radius;
  const float invRadius = 1.0f / static_cast<float>(radius);

  for (int offsetY = -radius; offsetY <= radius; ++offsetY) {
    const int y = centerY + offsetY;
    if (y <= 0 || y >= height_ - 1) {
      continue;
    }
    for (int offsetX = -radius; offsetX <= radius; ++offsetX) {
      const int x = centerX + offsetX;
      if (x <= 0 || x >= width_ - 1) {
        continue;
      }
      const int distSq = offsetX * offsetX + offsetY * offsetY;
      if (distSq > radiusSq) {
        continue;
      }
      const float distance = std::sqrt(static_cast<float>(distSq)) * invRadius;
      const float profile = 0.5f * (std::cos(distance * kPi) + 1.0f);
      const float scaled = (static_cast<float>(heightDelta) * profile) / 8.0f;
      const std::size_t index = static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) +
                                static_cast<std::size_t>(x);
      buffer[index] += static_cast<int>(std::round(scaled));
    }
  }
}

void WaterBump::applyHeightBlob(int centerX, int centerY, int radius, int heightDelta) {
  if (radius <= 0) {
    return;
  }

  auto& buffer = heightBuffers_[currentPage_];
  const int radiusSq = radius * radius;

  for (int offsetY = -radius; offsetY <= radius; ++offsetY) {
    const int y = centerY + offsetY;
    if (y <= 0 || y >= height_ - 1) {
      continue;
    }
    for (int offsetX = -radius; offsetX <= radius; ++offsetX) {
      const int x = centerX + offsetX;
      if (x <= 0 || x >= width_ - 1) {
        continue;
      }
      const int distSq = offsetX * offsetX + offsetY * offsetY;
      if (distSq > radiusSq) {
        continue;
      }
      const float falloff = 1.0f - (static_cast<float>(distSq) / static_cast<float>(radiusSq));
      const float scaled = static_cast<float>(heightDelta) * falloff * 0.5f;
      const std::size_t index = static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) +
                                static_cast<std::size_t>(x);
      buffer[index] += static_cast<int>(std::round(scaled));
    }
  }
}

void WaterBump::renderFrame(const std::uint8_t* srcPixels) {
  const std::size_t totalPixels =
      static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_);
  if (scratch_.size() < totalPixels * kChannels) {
    scratch_.resize(totalPixels * kChannels);
  }

  const auto& heightMap = heightBuffers_[currentPage_];
  if (width_ < 3 || height_ < 3) {
    std::copy_n(srcPixels, totalPixels * kChannels, scratch_.data());
    return;
  }

  for (int y = 0; y < height_; ++y) {
    for (int x = 0; x < width_; ++x) {
      const std::size_t index = static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) +
                                static_cast<std::size_t>(x);
      const std::size_t pixelOffset = index * kChannels;

      if (y == 0 || y == height_ - 1 || x == 0 || x == width_ - 1) {
        std::copy_n(srcPixels + pixelOffset, kChannels, scratch_.data() + pixelOffset);
        continue;
      }

      const int heightCenter = heightMap[index];
      const int dx = heightCenter - heightMap[index + 1];
      const int dy = heightCenter - heightMap[index + static_cast<std::size_t>(width_)];

      int sampleX = x + (dx >> 3);
      int sampleY = y + (dy >> 3);
      sampleX = clampToRange(sampleX, 0, width_ - 1);
      sampleY = clampToRange(sampleY, 0, height_ - 1);
      const std::size_t sampleIndex =
          static_cast<std::size_t>(sampleY) * static_cast<std::size_t>(width_) +
          static_cast<std::size_t>(sampleX);
      const std::size_t sampleOffset = sampleIndex * kChannels;

      float r = static_cast<float>(srcPixels[sampleOffset + 0]);
      float g = static_cast<float>(srcPixels[sampleOffset + 1]);
      float b = static_cast<float>(srcPixels[sampleOffset + 2]);
      const std::uint8_t a = srcPixels[sampleOffset + 3];

      const int gradientX = heightMap[index + 1] - heightMap[index - 1];
      const int gradientY = heightMap[index + static_cast<std::size_t>(width_)] -
                            heightMap[index - static_cast<std::size_t>(width_)];

      float nx = -static_cast<float>(gradientX);
      float ny = -static_cast<float>(gradientY);
      float nz = kNormalScale;
      const float length = std::sqrt(nx * nx + ny * ny + nz * nz);
      if (length > 0.0f) {
        const float invLength = 1.0f / length;
        nx *= invLength;
        ny *= invLength;
        nz *= invLength;
      }

      float shade = nx * kLightDirection[0] + ny * kLightDirection[1] + nz * kLightDirection[2];
      shade = std::clamp(shade, -1.0f, 1.0f);
      const float intensity = std::clamp(kAmbient + kDiffuse * std::max(shade, 0.0f), 0.0f, 1.0f);

      r = std::clamp(r * intensity, 0.0f, 255.0f);
      g = std::clamp(g * intensity, 0.0f, 255.0f);
      b = std::clamp(b * intensity, 0.0f, 255.0f);

      scratch_[pixelOffset + 0] = static_cast<std::uint8_t>(std::lround(r));
      scratch_[pixelOffset + 1] = static_cast<std::uint8_t>(std::lround(g));
      scratch_[pixelOffset + 2] = static_cast<std::uint8_t>(std::lround(b));
      scratch_[pixelOffset + 3] = a;
    }
  }
}

void WaterBump::updateWater(int nextPage) {
  auto& newBuffer = heightBuffers_[nextPage];
  const auto& oldBuffer = heightBuffers_[currentPage_];

  if (width_ < 3 || height_ < 3) {
    newBuffer = oldBuffer;
    return;
  }

  const std::size_t width = static_cast<std::size_t>(width_);
  const std::size_t height = static_cast<std::size_t>(height_);

  for (std::size_t y = 1; y < height - 1; ++y) {
    const std::size_t row = y * width;
    for (std::size_t x = 1; x < width - 1; ++x) {
      const std::size_t index = row + x;
      const int sum = oldBuffer[index + 1] + oldBuffer[index - 1] + oldBuffer[index + width] +
                      oldBuffer[index - width] + oldBuffer[index + width + 1] +
                      oldBuffer[index + width - 1] + oldBuffer[index - width + 1] +
                      oldBuffer[index - width - 1];
      const int newHeight = (sum >> 2) - newBuffer[index];
      newBuffer[index] = newHeight - (newHeight >> density_);
    }
  }

  for (int x = 0; x < width_; ++x) {
    newBuffer[static_cast<std::size_t>(x)] = 0;
    newBuffer[(static_cast<std::size_t>(height_) - 1) * width + static_cast<std::size_t>(x)] = 0;
  }
  for (int y = 0; y < height_; ++y) {
    newBuffer[static_cast<std::size_t>(y) * width] = 0;
    newBuffer[static_cast<std::size_t>(y) * width + static_cast<std::size_t>(width_ - 1)] = 0;
  }
}

}  // namespace avs::effects::trans
