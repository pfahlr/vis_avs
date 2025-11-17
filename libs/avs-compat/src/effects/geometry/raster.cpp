#include "effects/geometry/raster.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <utility>

namespace avs::effects::geometry {

namespace {

inline bool inBounds(const FrameBufferView& fb, int x, int y) {
  return x >= 0 && y >= 0 && x < fb.width && y < fb.height;
}

inline std::uint8_t clampByte(int value) {
  if (value < 0) return 0;
  if (value > 255) return 255;
  return static_cast<std::uint8_t>(value);
}

inline std::uint8_t clampByte(float value) {
  if (value <= 0.0f) return 0;
  if (value >= 255.0f) return 255;
  return static_cast<std::uint8_t>(std::floor(value + 0.5f));
}

inline ColorRGBA8 premultiply(ColorRGBA8 c) { return c; }

}  // namespace

void clear(FrameBufferView& fb, const ColorRGBA8& color) {
  if (!fb.data || fb.width <= 0 || fb.height <= 0) return;
  for (int y = 0; y < fb.height; ++y) {
    std::uint8_t* row = fb.data + static_cast<std::size_t>(y) * fb.stride;
    for (int x = 0; x < fb.width; ++x) {
      row[x * 4 + 0] = color.r;
      row[x * 4 + 1] = color.g;
      row[x * 4 + 2] = color.b;
      row[x * 4 + 3] = color.a;
    }
  }
}

void copyFrom(FrameBufferView& dst, const FrameBufferView& src) {
  if (!dst.data || !src.data) return;
  int width = std::min(dst.width, src.width);
  int height = std::min(dst.height, src.height);
  for (int y = 0; y < height; ++y) {
    std::uint8_t* d = dst.data + static_cast<std::size_t>(y) * dst.stride;
    const std::uint8_t* s = src.data + static_cast<std::size_t>(y) * src.stride;
    std::copy_n(s, static_cast<std::size_t>(width) * 4u, d);
  }
}

void blendPixel(FrameBufferView& fb, int x, int y, const ColorRGBA8& color, std::uint8_t coverage) {
  if (!fb.data || !inBounds(fb, x, y)) return;
  std::uint8_t* p =
      fb.data + static_cast<std::size_t>(y) * fb.stride + static_cast<std::size_t>(x) * 4u;
  const std::uint8_t alpha = clampByte((static_cast<int>(coverage) * color.a + 127) / 255);
  if (alpha == 0) return;
  const int inv = 255 - alpha;
  p[0] = clampByte((p[0] * inv + color.r * alpha + 127) / 255);
  p[1] = clampByte((p[1] * inv + color.g * alpha + 127) / 255);
  p[2] = clampByte((p[2] * inv + color.b * alpha + 127) / 255);
  p[3] = clampByte(p[3] + alpha);
}

void drawHorizontalSpan(FrameBufferView& fb, int x0, int x1, int y, const ColorRGBA8& color) {
  if (!fb.data || y < 0 || y >= fb.height) return;
  if (x0 > x1) std::swap(x0, x1);
  x0 = std::max(0, x0);
  x1 = std::min(fb.width - 1, x1);
  for (int x = x0; x <= x1; ++x) {
    blendPixel(fb, x, y, color);
  }
}

void drawThickLine(FrameBufferView& fb, int x0, int y0, int x1, int y1, int thickness,
                   const ColorRGBA8& color) {
  if (thickness <= 1) {
    int dx = std::abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    while (true) {
      blendPixel(fb, x0, y0, color);
      if (x0 == x1 && y0 == y1) break;
      int e2 = err << 1;
      if (e2 >= dy) {
        err += dy;
        x0 += sx;
      }
      if (e2 <= dx) {
        err += dx;
        y0 += sy;
      }
    }
    return;
  }
  int radius = std::max(0, thickness / 2);
  int dx = std::abs(x1 - x0);
  int sx = x0 < x1 ? 1 : -1;
  int dy = -std::abs(y1 - y0);
  int sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  while (true) {
    for (int oy = -radius; oy <= radius; ++oy) {
      for (int ox = -radius; ox <= radius; ++ox) {
        if (ox * ox + oy * oy > radius * radius) continue;
        blendPixel(fb, x0 + ox, y0 + oy, color);
      }
    }
    if (x0 == x1 && y0 == y1) break;
    int e2 = err << 1;
    if (e2 >= dy) {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx) {
      err += dx;
      y0 += sy;
    }
  }
}

void fillRectangle(FrameBufferView& fb, int x, int y, int w, int h, const ColorRGBA8& color) {
  if (!fb.data) return;
  if (w < 0) {
    x += w + 1;
    w = -w;
  }
  if (h < 0) {
    y += h + 1;
    h = -h;
  }
  int x0 = std::max(0, x);
  int y0 = std::max(0, y);
  int x1 = std::min(fb.width - 1, x + w - 1);
  int y1 = std::min(fb.height - 1, y + h - 1);
  for (int yy = y0; yy <= y1; ++yy) {
    drawHorizontalSpan(fb, x0, x1, yy, color);
  }
}

void strokeRectangle(FrameBufferView& fb, int x, int y, int w, int h, int thickness,
                     const ColorRGBA8& color) {
  if (thickness <= 0) return;
  drawThickLine(fb, x, y, x + w - 1, y, thickness, color);
  drawThickLine(fb, x + w - 1, y, x + w - 1, y + h - 1, thickness, color);
  drawThickLine(fb, x + w - 1, y + h - 1, x, y + h - 1, thickness, color);
  drawThickLine(fb, x, y + h - 1, x, y, thickness, color);
}

void drawCircle(FrameBufferView& fb, int cx, int cy, int radius, const ColorRGBA8& color,
                bool filled, int thickness) {
  if (radius < 0 || !fb.data) return;
  if (filled || thickness <= 1) {
    int x = 0;
    int y = radius;
    int decision = 1 - radius;
    auto span = [&](int sx, int ex, int yy) {
      if (filled) {
        drawHorizontalSpan(fb, sx, ex, yy, color);
      } else {
        blendPixel(fb, sx, yy, color);
        if (sx != ex) blendPixel(fb, ex, yy, color);
      }
    };
    while (y >= x) {
      span(cx - y, cx + y, cy + x);
      span(cx - x, cx + x, cy + y);
      span(cx - y, cx + y, cy - x);
      span(cx - x, cx + x, cy - y);
      ++x;
      if (decision < 0) {
        decision += 2 * x + 1;
      } else {
        --y;
        decision += 2 * (x - y) + 1;
      }
    }
    return;
  }
  int x = radius;
  int y = 0;
  int err = 0;
  while (x >= y) {
    drawThickLine(fb, cx + x, cy + y, cx + x, cy + y, thickness, color);
    drawThickLine(fb, cx + y, cy + x, cx + y, cy + x, thickness, color);
    drawThickLine(fb, cx - y, cy + x, cx - y, cy + x, thickness, color);
    drawThickLine(fb, cx - x, cy + y, cx - x, cy + y, thickness, color);
    drawThickLine(fb, cx - x, cy - y, cx - x, cy - y, thickness, color);
    drawThickLine(fb, cx - y, cy - x, cx - y, cy - x, thickness, color);
    drawThickLine(fb, cx + y, cy - x, cx + y, cy - x, thickness, color);
    drawThickLine(fb, cx + x, cy - y, cx + x, cy - y, thickness, color);
    if (err <= 0) {
      ++y;
      err += 2 * y + 1;
    }
    if (err > 0) {
      --x;
      err -= 2 * x + 1;
    }
  }
}

void fillTriangle(FrameBufferView& fb, const Vec2i& p0, const Vec2i& p1, const Vec2i& p2,
                  const ColorRGBA8& color) {
  int minX = std::min({p0.x, p1.x, p2.x});
  int maxX = std::max({p0.x, p1.x, p2.x});
  int minY = std::min({p0.y, p1.y, p2.y});
  int maxY = std::max({p0.y, p1.y, p2.y});
  minX = std::max(0, minX);
  minY = std::max(0, minY);
  maxX = std::min(fb.width - 1, maxX);
  maxY = std::min(fb.height - 1, maxY);
  auto edge = [](const Vec2i& a, const Vec2i& b, int x, int y) {
    return static_cast<long long>(x - a.x) * (b.y - a.y) -
           static_cast<long long>(y - a.y) * (b.x - a.x);
  };
  for (int y = minY; y <= maxY; ++y) {
    for (int x = minX; x <= maxX; ++x) {
      long long e0 = edge(p0, p1, x, y);
      long long e1 = edge(p1, p2, x, y);
      long long e2 = edge(p2, p0, x, y);
      bool hasNeg = (e0 < 0) || (e1 < 0) || (e2 < 0);
      bool hasPos = (e0 > 0) || (e1 > 0) || (e2 > 0);
      if (!(hasNeg && hasPos)) {
        blendPixel(fb, x, y, color);
      }
    }
  }
}

void strokeTriangle(FrameBufferView& fb, const Vec2i& p0, const Vec2i& p1, const Vec2i& p2,
                    int thickness, const ColorRGBA8& color) {
  drawThickLine(fb, p0.x, p0.y, p1.x, p1.y, thickness, color);
  drawThickLine(fb, p1.x, p1.y, p2.x, p2.y, thickness, color);
  drawThickLine(fb, p2.x, p2.y, p0.x, p0.y, thickness, color);
}

void fillPolygon(FrameBufferView& fb, const std::vector<Vec2i>& points, const ColorRGBA8& color) {
  if (points.size() < 3) return;
  int minY = points[0].y;
  int maxY = points[0].y;
  for (const auto& p : points) {
    minY = std::min(minY, p.y);
    maxY = std::max(maxY, p.y);
  }
  minY = std::max(0, minY);
  maxY = std::min(fb.height - 1, maxY);
  std::vector<int> intersections;
  intersections.reserve(points.size());
  for (int y = minY; y <= maxY; ++y) {
    intersections.clear();
    for (std::size_t i = 0; i < points.size(); ++i) {
      const Vec2i& a = points[i];
      const Vec2i& b = points[(i + 1) % points.size()];
      if ((a.y <= y && b.y > y) || (b.y <= y && a.y > y)) {
        int dy = b.y - a.y;
        if (dy == 0) continue;
        int dx = b.x - a.x;
        int intersectX = a.x + (dx * (y - a.y)) / dy;
        intersections.push_back(intersectX);
      }
    }
    std::sort(intersections.begin(), intersections.end());
    for (std::size_t k = 0; k + 1 < intersections.size(); k += 2) {
      drawHorizontalSpan(fb, intersections[k], intersections[k + 1], y, color);
    }
  }
}

void strokePolygon(FrameBufferView& fb, const std::vector<Vec2i>& points, int thickness,
                   const ColorRGBA8& color) {
  if (points.size() < 2) return;
  for (std::size_t i = 0; i < points.size(); ++i) {
    const Vec2i& a = points[i];
    const Vec2i& b = points[(i + 1) % points.size()];
    drawThickLine(fb, a.x, a.y, b.x, b.y, thickness, color);
  }
}

ColorRGBA8 withAlpha(ColorRGBA8 color, int alpha) {
  if (alpha >= 0) color.a = clampByte(alpha);
  return color;
}

ColorRGBA8 makeColor(std::uint32_t rgb, int alpha) {
  ColorRGBA8 c;
  c.r = static_cast<std::uint8_t>((rgb >> 16) & 0xFFu);
  c.g = static_cast<std::uint8_t>((rgb >> 8) & 0xFFu);
  c.b = static_cast<std::uint8_t>(rgb & 0xFFu);
  c.a = clampByte(alpha);
  return c;
}

std::vector<Vec2i> parsePointList(std::string_view text) {
  std::vector<Vec2i> points;
  std::vector<int> values;
  const char* begin = text.data();
  const char* end = begin + text.size();
  const char* ptr = begin;
  while (ptr < end) {
    while (ptr < end && std::isspace(static_cast<unsigned char>(*ptr))) {
      ++ptr;
    }
    if (ptr >= end) break;
    const char* tokenStart = ptr;
    while (ptr < end && !std::isspace(static_cast<unsigned char>(*ptr)) && *ptr != ',' &&
           *ptr != ';') {
      ++ptr;
    }
    int value = 0;
    if (std::from_chars(tokenStart, ptr, value).ec == std::errc()) {
      values.push_back(value);
    }
    while (ptr < end && (*ptr == ',' || *ptr == ';')) ++ptr;
  }
  for (std::size_t i = 0; i + 1 < values.size(); i += 2) {
    points.push_back(Vec2i{values[i], values[i + 1]});
  }
  return points;
}

}  // namespace avs::effects::geometry
