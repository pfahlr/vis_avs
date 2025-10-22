#include "effects/trans/effect_scatter.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace avs::effects::trans {

namespace {
constexpr int kFalloffRadius = 4;
constexpr int kKernelSize = 8;
constexpr int kOffsetTableSize = 512;
constexpr int kOffsetMask = kOffsetTableSize - 1;
}  // namespace

bool Scatter::hasFramebuffer(const avs::core::RenderContext& context) {
  if (!context.framebuffer.data) {
    return false;
  }
  if (context.width <= 0 || context.height <= 0) {
    return false;
  }
  const std::size_t requiredBytes = static_cast<std::size_t>(context.width) *
                                    static_cast<std::size_t>(context.height) * 4u;
  return context.framebuffer.size >= requiredBytes;
}

std::uint32_t Scatter::lerpColor(std::uint32_t original,
                                 std::uint32_t scattered,
                                 int weight,
                                 int scale) {
  const int inv = scale - weight;
  const int bias = scale / 2;

  const auto blendChannel = [&](int shift) -> std::uint32_t {
    const int base = static_cast<int>((original >> shift) & 0xFFu);
    const int sample = static_cast<int>((scattered >> shift) & 0xFFu);
    const int value = base * inv + sample * weight + bias;
    const int normalized = value / scale;
    return static_cast<std::uint32_t>(std::clamp(normalized, 0, 255)) << shift;
  };

  return blendChannel(0) | blendChannel(8) | blendChannel(16) | blendChannel(24);
}

void Scatter::ensureScratch(std::size_t bytes) {
  if (scratch_.size() < bytes) {
    scratch_.resize(bytes);
  }
}

void Scatter::ensureOffsetTable(int width) {
  if (width <= 0) {
    offsets_.clear();
    cachedWidth_ = 0;
    return;
  }
  if (cachedWidth_ == width && static_cast<int>(offsets_.size()) == kOffsetTableSize) {
    return;
  }

  offsets_.resize(kOffsetTableSize);
  for (int i = 0; i < kOffsetTableSize; ++i) {
    int offsetX = (i % kKernelSize) - (kKernelSize / 2);
    int offsetY = ((i / kKernelSize) % kKernelSize) - (kKernelSize / 2);
    if (offsetX < 0) {
      ++offsetX;
    }
    if (offsetY < 0) {
      ++offsetY;
    }
    offsets_[i] = {offsetX, offsetY};
  }

  cachedWidth_ = width;
}

void Scatter::setParams(const avs::core::ParamBlock& params) {
  enabled_ = params.getBool("enabled", enabled_);
}

bool Scatter::render(avs::core::RenderContext& context) {
  if (!enabled_ || !hasFramebuffer(context)) {
    return true;
  }

  const int width = context.width;
  const int height = context.height;
  const int totalPixels = width * height;
  if (totalPixels <= 0) {
    return true;
  }

  const std::size_t bytes = static_cast<std::size_t>(totalPixels) * 4u;
  ensureScratch(bytes);
  std::uint8_t* framebuffer = context.framebuffer.data;
  std::memcpy(scratch_.data(), framebuffer, bytes);
  const auto* source = reinterpret_cast<const std::uint32_t*>(scratch_.data());
  auto* destination = reinterpret_cast<std::uint32_t*>(framebuffer);

  ensureOffsetTable(width);
  if (offsets_.empty()) {
    return true;
  }

  for (int y = 0; y < height; ++y) {
    const int distanceY = std::min(y, height - 1 - y);
    for (int x = 0; x < width; ++x) {
      const int index = y * width + x;
      const std::uint32_t basePixel = source[static_cast<std::size_t>(index)];
      const int distanceX = std::min(x, width - 1 - x);
      const int distance = std::max(0, std::min(distanceX, distanceY));
      const int weight = std::min(distance, kFalloffRadius);
      if (weight <= 0) {
        destination[static_cast<std::size_t>(index)] = basePixel;
        continue;
      }

      const std::uint32_t randomValue = context.rng.nextUint32();
      const int tableIndex = static_cast<int>(randomValue & kOffsetMask);
      const ScatterOffset& offset = offsets_[tableIndex];
      const int sampleX = std::clamp(x + offset.dx, 0, width - 1);
      const int sampleY = std::clamp(y + offset.dy, 0, height - 1);
      const int sampleIndex = sampleY * width + sampleX;
      const std::uint32_t scatteredPixel = source[static_cast<std::size_t>(sampleIndex)];

      if (weight >= kFalloffRadius) {
        destination[static_cast<std::size_t>(index)] = scatteredPixel;
      } else {
        destination[static_cast<std::size_t>(index)] =
            lerpColor(basePixel, scatteredPixel, weight, kFalloffRadius);
      }
    }
  }

  return true;
}

}  // namespace avs::effects::trans

