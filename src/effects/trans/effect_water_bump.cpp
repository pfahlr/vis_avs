#include "effects/trans/effect_water_bump.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace avs::effects::trans {

namespace {
constexpr int kDensityMin = 1;
constexpr int kDensityMax = 12;
}  // namespace

bool WaterBump::hasFramebuffer(const avs::core::RenderContext& context) {
  if (!context.framebuffer.data) {
    return false;
  }
  if (context.width <= 0 || context.height <= 0) {
    return false;
  }
  const std::size_t pixelCount =
      static_cast<std::size_t>(context.width) * static_cast<std::size_t>(context.height);
  const std::size_t requiredBytes = pixelCount * sizeof(std::uint32_t);
  return context.framebuffer.size >= requiredBytes;
}

int WaterBump::clamp(int value, int minValue, int maxValue) {
  return std::clamp(value, minValue, maxValue);
}

void WaterBump::setParams(const avs::core::ParamBlock& params) {
  enabled_ = params.getBool("enabled", enabled_);

  int density = params.getInt("density", density_);
  if (params.contains("damp")) {
    density = params.getInt("damp", density);
  } else if (params.contains("damping")) {
    density = params.getInt("damping", density);
  }
  density_ = clamp(density, kDensityMin, kDensityMax);

  int depth = params.getInt("depth", depth_);
  depth = std::clamp(depth, 0, 4096);
  depth_ = depth;

  bool randomDrop = params.getBool("random_drop", randomDrop_);
  if (!params.contains("random_drop") && params.contains("randomdrop")) {
    randomDrop = params.getBool("randomdrop", randomDrop);
  }
  randomDrop_ = randomDrop;

  int dropX = params.getInt("drop_position_x", dropPositionX_);
  if (params.contains("dropx")) {
    dropX = params.getInt("dropx", dropX);
  }
  dropPositionX_ = clamp(dropX, 0, 2);

  int dropY = params.getInt("drop_position_y", dropPositionY_);
  if (params.contains("dropy")) {
    dropY = params.getInt("dropy", dropY);
  }
  dropPositionY_ = clamp(dropY, 0, 2);

  int radius = params.getInt("drop_radius", dropRadius_);
  if (params.contains("radius")) {
    radius = params.getInt("radius", radius);
  }
  dropRadius_ = std::max(0, radius);

  int method = params.getInt("method", method_);
  method_ = clamp(method, 0, 1);
}

bool WaterBump::render(avs::core::RenderContext& context) {
  if (!enabled_ || !hasFramebuffer(context)) {
    return true;
  }

  ensureBuffers(context.width, context.height);
  if (width_ <= 0 || height_ <= 0) {
    return true;
  }

  if (context.audioBeat) {
    spawnBeatDrop(context);
  }

  const std::size_t pixelCount =
      static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_);
  if (pixelCount == 0) {
    return true;
  }

  const std::size_t requiredBytes = pixelCount * sizeof(std::uint32_t);
  if (scratch_.size() < requiredBytes) {
    scratch_.resize(requiredBytes);
  }

  std::uint8_t* framebuffer = context.framebuffer.data;
  std::memcpy(scratch_.data(), framebuffer, requiredBytes);

  const auto* source = reinterpret_cast<const std::uint32_t*>(scratch_.data());
  auto* destination = reinterpret_cast<std::uint32_t*>(framebuffer);

  displaceFrame(source, destination, width_, height_);

  const int nextPage = page_ ^ 1;
  if (method_ == 1) {
    simulateSludge(page_, nextPage);
  } else {
    simulate(page_, nextPage);
  }
  page_ = nextPage;

  return true;
}

void WaterBump::ensureBuffers(int width, int height) {
  if (width <= 0 || height <= 0) {
    width_ = 0;
    height_ = 0;
    page_ = 0;
    for (auto& buffer : heightBuffers_) {
      buffer.clear();
    }
    scratch_.clear();
    return;
  }

  if (width_ == width && height_ == height) {
    const std::size_t expectedSize =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    if (heightBuffers_[0].size() == expectedSize && heightBuffers_[1].size() == expectedSize) {
      return;
    }
  }

  const std::size_t pixelCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
  for (auto& buffer : heightBuffers_) {
    buffer.assign(pixelCount, 0);
  }
  width_ = width;
  height_ = height;
  page_ = 0;
}

void WaterBump::spawnBeatDrop(avs::core::RenderContext& context) {
  if (depth_ <= 0) {
    return;
  }
  if (heightBuffers_[page_].empty()) {
    return;
  }

  if (randomDrop_) {
    const int maxDimension = std::max(width_, height_);
    int radius = (dropRadius_ * maxDimension) / 100;
    if (radius <= 0) {
      return;
    }
    applySineBlob(-1, -1, radius, -depth_, &context.rng);
  } else {
    if (dropRadius_ <= 0) {
      return;
    }
    int x = width_ / 2;
    switch (dropPositionX_) {
      case 0:
        x = width_ / 4;
        break;
      case 2:
        x = (width_ * 3) / 4;
        break;
      default:
        x = width_ / 2;
        break;
    }
    int y = height_ / 2;
    switch (dropPositionY_) {
      case 0:
        y = height_ / 4;
        break;
      case 2:
        y = (height_ * 3) / 4;
        break;
      default:
        y = height_ / 2;
        break;
    }
    x = clamp(x, 1, std::max(1, width_ - 2));
    y = clamp(y, 1, std::max(1, height_ - 2));
    applySineBlob(x, y, dropRadius_, -depth_, &context.rng);
  }
}

void WaterBump::applySineBlob(int x, int y, int radius, int height,
                              avs::core::DeterministicRng* rng) {
  if (radius <= 0) {
    return;
  }
  if (width_ < 3 || height_ < 3) {
    return;
  }

  auto& buffer = heightBuffers_[page_];
  if (buffer.empty()) {
    return;
  }

  const int w = width_;
  const int h = height_;

  if (x < 0) {
    const int range = w - 2 * radius - 1;
    if (range > 0 && rng != nullptr) {
      const std::uint32_t value = rng->nextUint32();
      x = 1 + radius + static_cast<int>(value % static_cast<std::uint32_t>(range));
    } else {
      x = clamp(w / 2, 1, std::max(1, w - 2));
    }
  }
  if (y < 0) {
    const int range = h - 2 * radius - 1;
    if (range > 0 && rng != nullptr) {
      const std::uint32_t value = rng->nextUint32();
      y = 1 + radius + static_cast<int>(value % static_cast<std::uint32_t>(range));
    } else {
      y = clamp(h / 2, 1, std::max(1, h - 2));
    }
  }

  x = clamp(x, 1, std::max(1, w - 2));
  y = clamp(y, 1, std::max(1, h - 2));

  const int radiusSquared = radius * radius;
  if (radiusSquared <= 0) {
    return;
  }

  const int minX = std::max(1, x - radius);
  const int maxX = std::min(w - 2, x + radius);
  const int minY = std::max(1, y - radius);
  const int maxY = std::min(h - 2, y + radius);
  if (minX > maxX || minY > maxY) {
    return;
  }

  const double scale = 1024.0 / static_cast<double>(radius);
  const double lengthSquared = scale * scale;

  for (int yy = minY; yy <= maxY; ++yy) {
    const int dy = yy - y;
    const int dySquared = dy * dy;
    const int baseIndex = yy * w;
    for (int xx = minX; xx <= maxX; ++xx) {
      const int dx = xx - x;
      const int distanceSquared = dx * dx + dySquared;
      if (distanceSquared >= radiusSquared) {
        continue;
      }
      const double dist = std::sqrt(static_cast<double>(distanceSquared) * lengthSquared);
      const double term = std::cos(dist) + 65535.0;
      const double delta = term * static_cast<double>(height) / 524288.0;
      const std::size_t index = static_cast<std::size_t>(baseIndex + xx);
      buffer[index] += static_cast<int>(std::lround(delta));
    }
  }
}

void WaterBump::simulate(int sourcePage, int destinationPage) {
  auto& dest = heightBuffers_[destinationPage];
  const auto& src = heightBuffers_[sourcePage];
  if (dest.size() != src.size()) {
    dest.resize(src.size());
  }

  if (width_ < 3 || height_ < 3) {
    std::fill(dest.begin(), dest.end(), 0);
    return;
  }

  const int width = width_;
  const int height = height_;
  const int dampingShift = clamp(density_, kDensityMin, kDensityMax);

  for (int y = 1; y < height - 1; ++y) {
    const int row = y * width;
    for (int x = 1; x < width - 1; ++x) {
      const int index = row + x;
      int newHeight = src[index + width] + src[index - width] + src[index + 1] + src[index - 1] +
                      src[index - width - 1] + src[index - width + 1] + src[index + width - 1] +
                      src[index + width + 1];
      newHeight >>= 2;
      newHeight -= dest[index];
      dest[index] = newHeight - (newHeight >> dampingShift);
    }
  }

  for (int x = 0; x < width; ++x) {
    dest[x] = 0;
    dest[(height - 1) * width + x] = 0;
  }
  for (int y = 0; y < height; ++y) {
    dest[y * width] = 0;
    dest[y * width + (width - 1)] = 0;
  }
}

void WaterBump::simulateSludge(int sourcePage, int destinationPage) {
  auto& dest = heightBuffers_[destinationPage];
  const auto& src = heightBuffers_[sourcePage];
  if (dest.size() != src.size()) {
    dest.resize(src.size());
  }

  if (width_ < 3 || height_ < 3) {
    std::fill(dest.begin(), dest.end(), 0);
    return;
  }

  const int width = width_;
  const int height = height_;
  const int dampingShift = clamp(density_, kDensityMin, kDensityMax);

  for (int y = 1; y < height - 1; ++y) {
    const int row = y * width;
    for (int x = 1; x < width - 1; ++x) {
      const int index = row + x;
      int newHeight =
          (src[index] << 2) + src[index - 1 - width] + src[index + 1 - width] +
          src[index - 1 + width] + src[index + 1 + width] +
          ((src[index - 1] + src[index + 1] + src[index - width] + src[index + width]) << 1);
      newHeight -= (newHeight >> 6);
      dest[index] = newHeight >> dampingShift;
    }
  }

  for (int x = 0; x < width; ++x) {
    dest[x] = 0;
    dest[(height - 1) * width + x] = 0;
  }
  for (int y = 0; y < height; ++y) {
    dest[y * width] = 0;
    dest[y * width + (width - 1)] = 0;
  }
}

void WaterBump::displaceFrame(const std::uint32_t* source, std::uint32_t* destination, int width,
                              int height) const {
  const auto& heightMap = heightBuffers_[page_];
  const std::size_t expectedSize =
      static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
  if (heightMap.size() != expectedSize) {
    std::copy_n(source, expectedSize, destination);
    return;
  }

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const int index = y * width + x;
      if (x <= 0 || x >= width - 1 || y <= 0 || y >= height - 1) {
        destination[index] = source[index];
        continue;
      }

      const int heightValue = heightMap[static_cast<std::size_t>(index)];
      const int dx = heightValue - heightMap[static_cast<std::size_t>(index + 1)];
      const int dy = heightValue - heightMap[static_cast<std::size_t>(index + width)];

      int sampleX = x + (dx >> 3);
      int sampleY = y + (dy >> 3);
      sampleX = std::clamp(sampleX, 0, width - 1);
      sampleY = std::clamp(sampleY, 0, height - 1);
      const int sampleIndex = sampleY * width + sampleX;
      destination[index] = source[static_cast<std::size_t>(sampleIndex)];
    }
  }
}

}  // namespace avs::effects::trans
