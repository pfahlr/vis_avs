#include "effects/trans/effect_scatter.h"

#include <algorithm>
#include <cstddef>
#include <cstring>

namespace avs::effects::trans {

Scatter::Scatter() {
  for (int i = 0; i < kNeighbourTableSize; ++i) {
    int xp = (i % 8) - 4;
    if (xp < 0) {
      ++xp;
    }
    int yp = ((i / 8) % 8) - 4;
    if (yp < 0) {
      ++yp;
    }
    offsetX_[i] = xp;
    offsetY_[i] = yp;
  }
}

void Scatter::setParams(const avs::core::ParamBlock& params) {
  enabled_ = params.getBool("enabled", enabled_);
}

bool Scatter::render(avs::core::RenderContext& context) {
  if (!enabled_) {
    return true;
  }
  if (context.framebuffer.data == nullptr || context.width <= 0 || context.height <= 0) {
    return true;
  }

  const int width = context.width;
  const int height = context.height;
  if (height <= kBorderRows * 2) {
    return true;
  }

  const std::size_t totalPixels =
      static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
  const std::size_t totalBytes = totalPixels * 4u;
  if (scratch_.size() < totalBytes) {
    scratch_.resize(totalBytes);
  }

  std::uint8_t* dst = context.framebuffer.data;
  std::memcpy(scratch_.data(), dst, totalBytes);

  const int maxX = width - 1;
  const int maxY = height - 1;

  for (int y = kBorderRows; y < height - kBorderRows; ++y) {
    for (int x = 0; x < width; ++x) {
      const std::size_t dstIndex = (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                                    static_cast<std::size_t>(x)) *
                                   4u;
      const std::uint32_t randomValue = context.rng.nextUint32();
      const int tableIndex = static_cast<int>(randomValue & (kNeighbourTableSize - 1));
      int srcX = x + offsetX_[tableIndex];
      int srcY = y + offsetY_[tableIndex];
      srcX = std::clamp(srcX, 0, maxX);
      srcY = std::clamp(srcY, 0, maxY);
      const std::size_t srcIndex =
          (static_cast<std::size_t>(srcY) * static_cast<std::size_t>(width) +
           static_cast<std::size_t>(srcX)) *
          4u;
      dst[dstIndex + 0] = scratch_[srcIndex + 0];
      dst[dstIndex + 1] = scratch_[srcIndex + 1];
      dst[dstIndex + 2] = scratch_[srcIndex + 2];
      dst[dstIndex + 3] = scratch_[srcIndex + 3];
    }
  }

  return true;
}

}  // namespace avs::effects::trans
