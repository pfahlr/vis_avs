#pragma once

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <avs/core/RenderContext.hpp>
#include <avs/runtime/GlobalState.hpp>

namespace avs::effects::detail {

struct RGBA {
  std::uint8_t r{0};
  std::uint8_t g{0};
  std::uint8_t b{0};
  std::uint8_t a{255};
};

inline RGBA colorFromInt(int value, std::uint8_t defaultAlpha = 255) {
  const std::uint32_t v = static_cast<std::uint32_t>(value);
  return RGBA{static_cast<std::uint8_t>((v >> 16) & 0xFFu),
              static_cast<std::uint8_t>((v >> 8) & 0xFFu),
              static_cast<std::uint8_t>(v & 0xFFu), defaultAlpha};
}

inline std::uint8_t clampByte(int value) {
  return static_cast<std::uint8_t>(std::clamp(value, 0, 255));
}

inline std::uint8_t scaleChannel(std::uint8_t value, std::uint8_t alpha) {
  return static_cast<std::uint8_t>((static_cast<int>(value) * static_cast<int>(alpha) + 127) / 255);
}

inline bool inBounds(const avs::core::RenderContext& ctx, int x, int y) {
  return x >= 0 && y >= 0 && x < ctx.width && y < ctx.height;
}

inline std::uint8_t saturatingAdd(std::uint8_t a, std::uint8_t b) {
  const std::uint16_t sum = static_cast<std::uint16_t>(a) + static_cast<std::uint16_t>(b);
  return static_cast<std::uint8_t>(std::min<std::uint16_t>(sum, 255u));
}

inline std::uint8_t channelMax(std::uint8_t a, std::uint8_t b) {
  return std::max(a, b);
}

inline std::uint8_t channelMin(std::uint8_t a, std::uint8_t b) {
  return std::min(a, b);
}

inline std::uint8_t subtractClamp(std::uint8_t a, std::uint8_t b) {
  return static_cast<std::uint8_t>(a > b ? a - b : 0u);
}

inline std::uint8_t averageChannel(std::uint8_t a, std::uint8_t b) {
  return static_cast<std::uint8_t>((static_cast<int>(a) + static_cast<int>(b) + 1) / 2);
}

inline std::uint8_t multiplyChannel(std::uint8_t a, std::uint8_t b) {
  return static_cast<std::uint8_t>((static_cast<int>(a) * static_cast<int>(b) + 127) / 255);
}

inline std::uint8_t blendAdjustChannel(std::uint8_t dst, std::uint8_t src, std::uint8_t alpha) {
  const std::uint8_t inv = static_cast<std::uint8_t>(255u - alpha);
  return static_cast<std::uint8_t>((static_cast<int>(dst) * inv + static_cast<int>(src) * alpha + 127) / 255);
}

inline const avs::runtime::LegacyRenderState* legacyRenderState(const avs::core::RenderContext& ctx) {
  if (ctx.globals && ctx.globals->legacyRender.lineBlendModeActive) {
    return &ctx.globals->legacyRender;
  }
  return nullptr;
}

inline std::optional<int> legacyLineWidthOverride(const avs::core::RenderContext& ctx) {
  if (const auto* legacy = legacyRenderState(ctx)) {
    const int width = static_cast<int>((legacy->lineBlendMode >> 16) & 0xFFu);
    if (width > 0) {
      return width;
    }
  }
  return std::nullopt;
}

inline void blendPixel(const avs::core::RenderContext& ctx,
                       int x,
                       int y,
                       const RGBA& color,
                       std::uint8_t coverage = 255) {
  if (!ctx.framebuffer.data || !inBounds(ctx, x, y)) {
    return;
  }
  auto* pixel = ctx.framebuffer.data + (static_cast<std::size_t>(y) * ctx.width + x) * 4;
  const std::uint8_t effectiveAlpha = static_cast<std::uint8_t>((static_cast<int>(coverage) * color.a + 127) / 255);
  if (effectiveAlpha == 0) {
    return;
  }
  const auto* legacy = legacyRenderState(ctx);
  if (!legacy) {
    const int inv = 255 - effectiveAlpha;
    pixel[0] = static_cast<std::uint8_t>((pixel[0] * inv + color.r * effectiveAlpha + 127) / 255);
    pixel[1] = static_cast<std::uint8_t>((pixel[1] * inv + color.g * effectiveAlpha + 127) / 255);
    pixel[2] = static_cast<std::uint8_t>((pixel[2] * inv + color.b * effectiveAlpha + 127) / 255);
    pixel[3] = static_cast<std::uint8_t>(std::min(255, static_cast<int>(pixel[3]) + effectiveAlpha));
    return;
  }

  const std::uint8_t mode = static_cast<std::uint8_t>(legacy->lineBlendMode & 0xFFu);
  std::array<std::uint8_t, 4> source = {scaleChannel(color.r, effectiveAlpha),
                                        scaleChannel(color.g, effectiveAlpha),
                                        scaleChannel(color.b, effectiveAlpha),
                                        effectiveAlpha};

  switch (mode) {
    case 0:  // Replace
      pixel[0] = source[0];
      pixel[1] = source[1];
      pixel[2] = source[2];
      pixel[3] = source[3];
      break;
    case 1:  // Additive
      pixel[0] = saturatingAdd(pixel[0], source[0]);
      pixel[1] = saturatingAdd(pixel[1], source[1]);
      pixel[2] = saturatingAdd(pixel[2], source[2]);
      pixel[3] = saturatingAdd(pixel[3], source[3]);
      break;
    case 2:  // Maximum
      pixel[0] = channelMax(pixel[0], source[0]);
      pixel[1] = channelMax(pixel[1], source[1]);
      pixel[2] = channelMax(pixel[2], source[2]);
      pixel[3] = channelMax(pixel[3], source[3]);
      break;
    case 3:  // 50/50 blend
      pixel[0] = averageChannel(pixel[0], source[0]);
      pixel[1] = averageChannel(pixel[1], source[1]);
      pixel[2] = averageChannel(pixel[2], source[2]);
      pixel[3] = averageChannel(pixel[3], source[3]);
      break;
    case 4:  // Subtractive 1
      pixel[0] = subtractClamp(pixel[0], source[0]);
      pixel[1] = subtractClamp(pixel[1], source[1]);
      pixel[2] = subtractClamp(pixel[2], source[2]);
      pixel[3] = subtractClamp(pixel[3], source[3]);
      break;
    case 5:  // Subtractive 2
      pixel[0] = subtractClamp(source[0], pixel[0]);
      pixel[1] = subtractClamp(source[1], pixel[1]);
      pixel[2] = subtractClamp(source[2], pixel[2]);
      pixel[3] = subtractClamp(source[3], pixel[3]);
      break;
    case 6:  // Multiply
      pixel[0] = multiplyChannel(pixel[0], source[0]);
      pixel[1] = multiplyChannel(pixel[1], source[1]);
      pixel[2] = multiplyChannel(pixel[2], source[2]);
      pixel[3] = multiplyChannel(pixel[3], source[3]);
      break;
    case 7:  // Adjustable blend
      pixel[0] = blendAdjustChannel(pixel[0], source[0], static_cast<std::uint8_t>((legacy->lineBlendMode >> 8) & 0xFFu));
      pixel[1] = blendAdjustChannel(pixel[1], source[1], static_cast<std::uint8_t>((legacy->lineBlendMode >> 8) & 0xFFu));
      pixel[2] = blendAdjustChannel(pixel[2], source[2], static_cast<std::uint8_t>((legacy->lineBlendMode >> 8) & 0xFFu));
      pixel[3] = blendAdjustChannel(pixel[3], source[3], static_cast<std::uint8_t>((legacy->lineBlendMode >> 8) & 0xFFu));
      break;
    case 8:  // XOR
      pixel[0] ^= source[0];
      pixel[1] ^= source[1];
      pixel[2] ^= source[2];
      pixel[3] ^= source[3];
      break;
    case 9:  // Minimum
      pixel[0] = channelMin(pixel[0], source[0]);
      pixel[1] = channelMin(pixel[1], source[1]);
      pixel[2] = channelMin(pixel[2], source[2]);
      pixel[3] = channelMin(pixel[3], source[3]);
      break;
    default:
      // Fallback to alpha blend when mode is unknown
      const int inv = 255 - effectiveAlpha;
      pixel[0] = static_cast<std::uint8_t>((pixel[0] * inv + color.r * effectiveAlpha + 127) / 255);
      pixel[1] = static_cast<std::uint8_t>((pixel[1] * inv + color.g * effectiveAlpha + 127) / 255);
      pixel[2] = static_cast<std::uint8_t>((pixel[2] * inv + color.b * effectiveAlpha + 127) / 255);
      pixel[3] = static_cast<std::uint8_t>(std::min(255, static_cast<int>(pixel[3]) + effectiveAlpha));
      break;
  }
}

struct Point {
  int x{0};
  int y{0};
};

inline bool isDelimiter(char c) {
  switch (c) {
    case ' ': case '\t': case '\n': case '\r': case ',': case ';':
      return true;
    default:
      return false;
  }
}

inline std::vector<Point> parsePointList(std::string_view text) {
  std::vector<Point> points;
  std::vector<int> values;
  const char* begin = text.data();
  const char* end = begin + text.size();
  const char* ptr = begin;
  while (ptr < end) {
    while (ptr < end && isDelimiter(*ptr)) {
      ++ptr;
    }
    if (ptr >= end) {
      break;
    }
    const char* tokenStart = ptr;
    while (ptr < end && !isDelimiter(*ptr)) {
      ++ptr;
    }
    int value = 0;
    auto res = std::from_chars(tokenStart, ptr, value);
    if (res.ec == std::errc()) {
      values.push_back(value);
    }
  }
  for (std::size_t i = 0; i + 1 < values.size(); i += 2) {
    points.push_back(Point{values[i], values[i + 1]});
  }
  return points;
}

inline void drawHorizontalSpan(const avs::core::RenderContext& ctx,
                               int x0,
                               int x1,
                               int y,
                               const RGBA& color) {
  if (y < 0 || y >= ctx.height) {
    return;
  }
  if (x0 > x1) std::swap(x0, x1);
  x0 = std::max(0, x0);
  x1 = std::min(ctx.width - 1, x1);
  for (int x = x0; x <= x1; ++x) {
    blendPixel(ctx, x, y, color);
  }
}

inline void drawFilledCircle(const avs::core::RenderContext& ctx,
                             int cx,
                             int cy,
                             int radius,
                             const RGBA& color) {
  if (radius < 0) {
    return;
  }
  int x = 0;
  int y = radius;
  int decision = 1 - radius;
  while (y >= x) {
    drawHorizontalSpan(ctx, cx - y, cx + y, cy + x, color);
    drawHorizontalSpan(ctx, cx - x, cx + x, cy + y, color);
    drawHorizontalSpan(ctx, cx - y, cx + y, cy - x, color);
    drawHorizontalSpan(ctx, cx - x, cx + x, cy - y, color);
    ++x;
    if (decision < 0) {
      decision += 2 * x + 1;
    } else {
      --y;
      decision += 2 * (x - y) + 1;
    }
  }
}

inline void drawThickLine(const avs::core::RenderContext& ctx,
                          int x0,
                          int y0,
                          int x1,
                          int y1,
                          int width,
                          const RGBA& color) {
  int dx = std::abs(x1 - x0);
  int dy = std::abs(y1 - y0);
  int sx = (x0 < x1) ? 1 : -1;
  int sy = (y0 < y1) ? 1 : -1;
  int err = dx - dy;
  const int radius = std::max(0, width / 2);
  while (true) {
    if (radius == 0) {
      blendPixel(ctx, x0, y0, color);
    } else {
      drawFilledCircle(ctx, x0, y0, radius, color);
    }
    if (x0 == x1 && y0 == y1) {
      break;
    }
    int e2 = 2 * err;
    if (e2 > -dy) {
      err -= dy;
      x0 += sx;
    }
    if (e2 < dx) {
      err += dx;
      y0 += sy;
    }
  }
}

inline int64_t edgeFunction(const Point& a, const Point& b, int px, int py) {
  return static_cast<int64_t>(px - a.x) * (b.y - a.y) -
         static_cast<int64_t>(py - a.y) * (b.x - a.x);
}

inline bool pointInTriangle(const Point& p0, const Point& p1, const Point& p2, int x, int y) {
  const int64_t e0 = edgeFunction(p0, p1, x, y);
  const int64_t e1 = edgeFunction(p1, p2, x, y);
  const int64_t e2 = edgeFunction(p2, p0, x, y);
  const bool hasNeg = (e0 < 0) || (e1 < 0) || (e2 < 0);
  const bool hasPos = (e0 > 0) || (e1 > 0) || (e2 > 0);
  return !(hasNeg && hasPos);
}

inline void dilateMask(std::vector<std::uint8_t>& mask,
                       int width,
                       int height,
                       int radius) {
  if (radius <= 0) {
    return;
  }
  std::vector<std::uint8_t> original = mask;
  const std::array<Point, 8> offsets{{{-radius, 0},
                                      {radius, 0},
                                      {0, -radius},
                                      {0, radius},
                                      {-radius, -radius},
                                      {radius, -radius},
                                      {radius, radius},
                                      {-radius, radius}}};
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (!original[static_cast<std::size_t>(y) * width + x]) {
        continue;
      }
      for (const auto& off : offsets) {
        const int nx = x + off.x;
        const int ny = y + off.y;
        if (nx < 0 || ny < 0 || nx >= width || ny >= height) {
          continue;
        }
        auto& dst = mask[static_cast<std::size_t>(ny) * width + nx];
        dst = std::max<std::uint8_t>(dst, original[static_cast<std::size_t>(y) * width + x]);
      }
    }
  }
}

inline std::vector<std::uint8_t> createStrokeMask(const std::vector<std::uint8_t>& base,
                                                  int width,
                                                  int height,
                                                  int radius) {
  if (radius <= 0) {
    return {};
  }
  std::vector<std::uint8_t> mask = base;
  dilateMask(mask, width, height, radius);
  for (std::size_t i = 0; i < mask.size(); ++i) {
    if (base[i] >= mask[i]) {
      mask[i] = 0;
    }
  }
  return mask;
}

inline void boxBlur(std::vector<std::uint8_t>& mask, int width, int height, int radius) {
  if (radius <= 0 || mask.empty()) {
    return;
  }
  const int stride = width + 1;
  std::vector<int> integral(static_cast<std::size_t>(stride) * (height + 1), 0);
  for (int y = 0; y < height; ++y) {
    int rowSum = 0;
    for (int x = 0; x < width; ++x) {
      rowSum += mask[static_cast<std::size_t>(y) * width + x];
      integral[static_cast<std::size_t>(y + 1) * stride + (x + 1)] =
          integral[static_cast<std::size_t>(y) * stride + (x + 1)] + rowSum;
    }
  }
  std::vector<std::uint8_t> output(mask.size(), 0);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const int x0 = std::max(0, x - radius);
      const int y0 = std::max(0, y - radius);
      const int x1 = std::min(width, x + radius + 1);
      const int y1 = std::min(height, y + radius + 1);
      const int sum = integral[static_cast<std::size_t>(y1) * stride + x1] -
                      integral[static_cast<std::size_t>(y0) * stride + x1] -
                      integral[static_cast<std::size_t>(y1) * stride + x0] +
                      integral[static_cast<std::size_t>(y0) * stride + x0];
      const int area = (x1 - x0) * (y1 - y0);
      output[static_cast<std::size_t>(y) * width + x] = static_cast<std::uint8_t>(sum / area);
    }
  }
  mask.swap(output);
}

}  // namespace avs::effects::detail

