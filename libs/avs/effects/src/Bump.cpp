#include "avs/effects/Bump.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include "avs/runtime/GlobalState.hpp"

namespace avs::effects {

namespace {
constexpr int kChannels = 4;

std::array<std::uint8_t, kChannels> sampleColor(const std::vector<std::uint8_t>& src,
                                                int width,
                                                int height,
                                                float x,
                                                float y) {
  std::array<std::uint8_t, kChannels> color{};
  if (width <= 0 || height <= 0 || src.empty()) {
    return color;
  }

  x = std::clamp(x, 0.0f, static_cast<float>(width - 1));
  y = std::clamp(y, 0.0f, static_cast<float>(height - 1));

  const int x0 = static_cast<int>(std::floor(x));
  const int y0 = static_cast<int>(std::floor(y));
  const int x1 = std::min(x0 + 1, width - 1);
  const int y1 = std::min(y0 + 1, height - 1);
  const float tx = x - static_cast<float>(x0);
  const float ty = y - static_cast<float>(y0);

  auto read = [&](int ix, int iy) {
    const std::size_t idx =
        (static_cast<std::size_t>(iy) * static_cast<std::size_t>(width) + static_cast<std::size_t>(ix)) *
        kChannels;
    return std::array<float, kChannels>{static_cast<float>(src[idx + 0u]), static_cast<float>(src[idx + 1u]),
                                        static_cast<float>(src[idx + 2u]), static_cast<float>(src[idx + 3u])};
  };

  const auto c00 = read(x0, y0);
  const auto c10 = read(x1, y0);
  const auto c01 = read(x0, y1);
  const auto c11 = read(x1, y1);

  for (int i = 0; i < kChannels; ++i) {
    const float a = c00[i] * (1.0f - tx) + c10[i] * tx;
    const float b = c01[i] * (1.0f - tx) + c11[i] * tx;
    const float value = a * (1.0f - ty) + b * ty;
    color[static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(std::clamp(std::lround(value), 0l, 255l));
  }
  return color;
}

float sampleFrameHeight(const std::vector<std::uint8_t>& src, int width, int height, int x, int y) {
  if (width <= 0 || height <= 0 || src.empty()) {
    return 0.5f;
  }
  const std::size_t idx =
      (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)) * kChannels;
  const float r = static_cast<float>(src[idx + 0u]) / 255.0f;
  const float g = static_cast<float>(src[idx + 1u]) / 255.0f;
  const float b = static_cast<float>(src[idx + 2u]) / 255.0f;
  return (r + g + b) / 3.0f;
}

float sampleHeightmap(const avs::runtime::Heightmap& map, float x, float y) {
  if (!map.valid()) {
    return 0.5f;
  }
  const float maxX = static_cast<float>(std::max(map.width - 1, 0));
  const float maxY = static_cast<float>(std::max(map.height - 1, 0));
  x = std::clamp(x, 0.0f, maxX);
  y = std::clamp(y, 0.0f, maxY);
  const int x0 = static_cast<int>(std::floor(x));
  const int y0 = static_cast<int>(std::floor(y));
  const int x1 = std::min(x0 + 1, map.width - 1);
  const int y1 = std::min(y0 + 1, map.height - 1);
  const float tx = x - static_cast<float>(x0);
  const float ty = y - static_cast<float>(y0);

  auto read = [&](int ix, int iy) {
    const std::size_t idx =
        static_cast<std::size_t>(iy) * static_cast<std::size_t>(map.width) + static_cast<std::size_t>(ix);
    return map.samples[idx];
  };

  const float h00 = read(x0, y0);
  const float h10 = read(x1, y0);
  const float h01 = read(x0, y1);
  const float h11 = read(x1, y1);
  const float a = h00 * (1.0f - tx) + h10 * tx;
  const float b = h01 * (1.0f - tx) + h11 * tx;
  return std::clamp(a * (1.0f - ty) + b * ty, 0.0f, 1.0f);
}

}  // namespace

bool Bump::render(avs::core::RenderContext& context) {
  if (!context.framebuffer.data || context.width <= 0 || context.height <= 0) {
    return true;
  }

  const int width = context.width;
  const int height = context.height;
  const std::size_t pixelCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
  const std::size_t bufferSize = pixelCount * kChannels;
  std::vector<std::uint8_t> source(context.framebuffer.data, context.framebuffer.data + bufferSize);

  const avs::runtime::Heightmap* externalMap = nullptr;
  if (!useFrameHeightmap_ && context.globals && !heightmapKey_.empty()) {
    auto it = context.globals->heightmaps.find(heightmapKey_);
    if (it != context.globals->heightmaps.end() && it->second.valid()) {
      externalMap = &it->second;
    }
  }

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      float heightValue = midpoint_;
      if (externalMap) {
        const float u = width > 1 ? (static_cast<float>(x) / static_cast<float>(width - 1)) : 0.0f;
        const float v = height > 1 ? (static_cast<float>(y) / static_cast<float>(height - 1)) : 0.0f;
        const float hx = u * static_cast<float>(externalMap->width - 1);
        const float hy = v * static_cast<float>(externalMap->height - 1);
        heightValue = sampleHeightmap(*externalMap, hx, hy);
      } else {
        heightValue = sampleFrameHeight(source, width, height, x, y);
      }

      const float offsetX = (heightValue - midpoint_) * scaleX_;
      const float offsetY = (heightValue - midpoint_) * scaleY_;
      const float sampleX = static_cast<float>(x) + offsetX;
      const float sampleY = static_cast<float>(y) + offsetY;
      const auto color = sampleColor(source, width, height, sampleX, sampleY);

      const std::size_t idx =
          (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)) *
          kChannels;
      context.framebuffer.data[idx + 0u] = color[0];
      context.framebuffer.data[idx + 1u] = color[1];
      context.framebuffer.data[idx + 2u] = color[2];
      context.framebuffer.data[idx + 3u] = 255u;
    }
  }

  return true;
}

void Bump::setParams(const avs::core::ParamBlock& params) {
  scaleX_ = params.getFloat("scale_x", scaleX_);
  scaleY_ = params.getFloat("scale_y", scaleY_);
  midpoint_ = params.getFloat("midpoint", midpoint_);
  useFrameHeightmap_ = params.getBool("use_frame_heightmap", useFrameHeightmap_);
  if (params.contains("heightmap")) {
    heightmapKey_ = params.getString("heightmap", heightmapKey_);
  }
}

}  // namespace avs::effects

