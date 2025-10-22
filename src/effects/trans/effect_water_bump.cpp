#include "effects/trans/effect_water_bump.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <numbers>

namespace avs::effects::trans {

namespace {
constexpr std::size_t kChannels = 4u;
constexpr int kDisplacementShift = 3;
constexpr float kAmbientLight = 0.35f;
constexpr float kLightX = 0.35f;
constexpr float kLightY = 0.35f;
constexpr float kLightZ = 0.87f;
constexpr float kNormalZ = 64.0f;

int clampInt(int value, int minValue, int maxValue) {
  return std::max(minValue, std::min(maxValue, value));
}

bool hasFramebuffer(const avs::core::RenderContext& context, std::size_t requiredBytes) {
  return context.framebuffer.data != nullptr && context.framebuffer.size >= requiredBytes &&
         context.width > 0 && context.height > 0;
}

float normalizeSafe(float value, float minValue, float maxValue) {
  if (maxValue <= minValue) {
    return minValue;
  }
  return std::clamp(value, minValue, maxValue);
}

}  // namespace

void WaterBump::setParams(const avs::core::ParamBlock& params) {
  if (params.contains("enabled")) {
    enabled_ = params.getBool("enabled", enabled_);
  }
  if (params.contains("damping")) {
    damping_ = clampInt(params.getInt("damping", damping_), 1, 10);
  }
  if (params.contains("depth")) {
    dropDepth_ = clampInt(params.getInt("depth", dropDepth_), 0, 5000);
  }
  if (params.contains("random_drop")) {
    randomDrop_ = params.getBool("random_drop", randomDrop_);
  }
  if (params.contains("drop_position_x")) {
    dropPositionX_ = clampInt(params.getInt("drop_position_x", dropPositionX_), 0, 2);
  }
  if (params.contains("drop_position_y")) {
    dropPositionY_ = clampInt(params.getInt("drop_position_y", dropPositionY_), 0, 2);
  }
  if (params.contains("drop_radius")) {
    dropRadiusPercent_ = clampInt(params.getInt("drop_radius", dropRadiusPercent_), 1, 100);
  }
  if (params.contains("bump_strength")) {
    bumpStrength_ = normalizeSafe(params.getFloat("bump_strength", bumpStrength_), 0.1f, 4.0f);
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
  if (!hasFramebuffer(context, requiredBytes)) {
    return true;
  }

  ensureState(context.width, context.height);

  if (context.audioBeat) {
    injectDrop(context);
  }

  const std::uint8_t* src = context.framebuffer.data;
  auto& heights = heightBuffers_[currentPage_];
  output_.resize(requiredBytes);

  const int widthInt = context.width;
  const int heightInt = context.height;

  for (int y = 0; y < heightInt; ++y) {
    for (int x = 0; x < widthInt; ++x) {
      const int index = y * widthInt + x;
      const std::size_t byteOffset = static_cast<std::size_t>(index) * kChannels;

      if (x == 0 || y == 0 || x == widthInt - 1 || y == heightInt - 1) {
        output_[byteOffset + 0] = src[byteOffset + 0];
        output_[byteOffset + 1] = src[byteOffset + 1];
        output_[byteOffset + 2] = src[byteOffset + 2];
        output_[byteOffset + 3] = src[byteOffset + 3];
        continue;
      }

      const int center = heights[index];
      const int right = heights[index + 1];
      const int down = heights[index + widthInt];

      const int dx = center - right;
      const int dy = center - down;

      const int offsetX = clampInt(x + (dx >> kDisplacementShift), 0, widthInt - 1);
      const int offsetY = clampInt(y + (dy >> kDisplacementShift), 0, heightInt - 1);
      const int sampleIndex = offsetY * widthInt + offsetX;

      const std::size_t sampleByte = static_cast<std::size_t>(sampleIndex) * kChannels;
      const int left = heights[index - 1];
      const int up = heights[index - widthInt];
      const int gradX = right - left;
      const int gradY = down - up;

      const float normalX = static_cast<float>(gradX);
      const float normalY = static_cast<float>(gradY);
      const float normalZ = kNormalZ;
      const float normalLength =
          std::sqrt(normalX * normalX + normalY * normalY + normalZ * normalZ);
      float dot = (normalX * kLightX + normalY * kLightY + normalZ * kLightZ);
      if (normalLength > 0.0f) {
        dot /= normalLength;
      } else {
        dot = kLightZ;
      }

      dot = std::clamp(dot * bumpStrength_, -1.0f, 1.0f);
      float shading = (dot + 1.0f) * 0.5f;
      shading = kAmbientLight + shading * (1.0f - kAmbientLight);
      shading = std::clamp(shading, 0.0f, 1.0f);

      const int red = static_cast<int>(static_cast<float>(src[sampleByte + 0]) * shading);
      const int green = static_cast<int>(static_cast<float>(src[sampleByte + 1]) * shading);
      const int blue = static_cast<int>(static_cast<float>(src[sampleByte + 2]) * shading);

      output_[byteOffset + 0] = clampByte(red);
      output_[byteOffset + 1] = clampByte(green);
      output_[byteOffset + 2] = clampByte(blue);
      output_[byteOffset + 3] = src[sampleByte + 3];
    }
  }

  std::memcpy(context.framebuffer.data, output_.data(), requiredBytes);

  calcWater(1 - currentPage_);
  currentPage_ = 1 - currentPage_;

  return true;
}

void WaterBump::ensureState(int width, int height) {
  if (width_ == width && height_ == height) {
    return;
  }

  width_ = width;
  height_ = height;
  currentPage_ = 0;

  const std::size_t pixelCount =
      static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_);
  for (auto& buffer : heightBuffers_) {
    buffer.assign(pixelCount, 0);
  }
  output_.assign(pixelCount * kChannels, 0u);
}

void WaterBump::injectDrop(avs::core::RenderContext& context) {
  if (dropDepth_ == 0 || width_ <= 2 || height_ <= 2) {
    return;
  }

  const int maxDim = std::max(width_, height_);
  int radius = (maxDim * dropRadiusPercent_) / 100;
  radius = clampInt(radius, 1, std::max(1, maxDim / 2 - 1));

  int centerX = width_ / 2;
  int centerY = height_ / 2;

  if (randomDrop_) {
    const int minX = radius;
    const int maxX = std::max(radius, width_ - radius - 1);
    const int minY = radius;
    const int maxY = std::max(radius, height_ - radius - 1);

    if (maxX > minX) {
      const std::uint32_t value = context.rng.nextUint32();
      centerX = minX + static_cast<int>(value % static_cast<std::uint32_t>(maxX - minX + 1));
    } else {
      centerX = minX;
    }

    if (maxY > minY) {
      const std::uint32_t value = context.rng.nextUint32();
      centerY = minY + static_cast<int>(value % static_cast<std::uint32_t>(maxY - minY + 1));
    } else {
      centerY = minY;
    }
  } else {
    static constexpr std::array<float, 3> kPositions = {0.25f, 0.5f, 0.75f};
    centerX =
        clampInt(static_cast<int>(kPositions[dropPositionX_] * static_cast<float>(width_ - 1)),
                 radius, width_ - radius - 1);
    centerY =
        clampInt(static_cast<int>(kPositions[dropPositionY_] * static_cast<float>(height_ - 1)),
                 radius, height_ - radius - 1);
  }

  addSineBlob(centerX, centerY, radius, -dropDepth_);
}

void WaterBump::addSineBlob(int centerX, int centerY, int radius, int amplitude) {
  if (radius <= 0 || amplitude == 0) {
    return;
  }

  if (width_ <= 0 || height_ <= 0) {
    return;
  }

  auto& buffer = heightBuffers_[currentPage_];

  const int clampedRadius = std::min(radius, std::min(width_, height_) / 2);
  const int effectiveRadius = std::max(1, clampedRadius);
  const int radiusSquared = effectiveRadius * effectiveRadius;

  const int minX = clampInt(centerX - effectiveRadius, 1, width_ - 2);
  const int maxX = clampInt(centerX + effectiveRadius, 1, width_ - 2);
  const int minY = clampInt(centerY - effectiveRadius, 1, height_ - 2);
  const int maxY = clampInt(centerY + effectiveRadius, 1, height_ - 2);

  for (int y = minY; y <= maxY; ++y) {
    const int dy = y - centerY;
    for (int x = minX; x <= maxX; ++x) {
      const int dx = x - centerX;
      const int distanceSquared = dx * dx + dy * dy;
      if (distanceSquared > radiusSquared) {
        continue;
      }

      const float distance = std::sqrt(static_cast<float>(distanceSquared));
      const float normalized = distance / static_cast<float>(effectiveRadius);
      const float weight = 0.5f * (std::cos(normalized * std::numbers::pi_v<float>) + 1.0f);
      const int delta = static_cast<int>(static_cast<float>(amplitude) * weight);
      buffer[y * width_ + x] += delta;
    }
  }
}

void WaterBump::calcWater(int nextPage) {
  if (width_ < 3 || height_ < 3) {
    return;
  }

  auto& newBuffer = heightBuffers_[nextPage];
  const auto& oldBuffer = heightBuffers_[currentPage_];

  const int width = width_;
  const int height = height_;

  for (int y = 1; y < height - 1; ++y) {
    for (int x = 1; x < width - 1; ++x) {
      const int idx = y * width + x;
      const int sum = oldBuffer[idx + width] + oldBuffer[idx - width] + oldBuffer[idx + 1] +
                      oldBuffer[idx - 1] + oldBuffer[idx - width - 1] + oldBuffer[idx - width + 1] +
                      oldBuffer[idx + width - 1] + oldBuffer[idx + width + 1];

      int newHeight = (sum >> 2) - newBuffer[idx];
      newBuffer[idx] = newHeight - (newHeight >> damping_);
    }
  }

  // Reset the borders to keep the simulation stable.
  for (int x = 0; x < width; ++x) {
    newBuffer[x] = 0;
    newBuffer[(height - 1) * width + x] = 0;
  }
  for (int y = 0; y < height; ++y) {
    newBuffer[y * width] = 0;
    newBuffer[y * width + (width - 1)] = 0;
  }
}

std::uint8_t WaterBump::clampByte(int value) {
  if (value <= 0) {
    return 0u;
  }
  if (value >= 255) {
    return 255u;
  }
  return static_cast<std::uint8_t>(value);
}

}  // namespace avs::effects::trans
