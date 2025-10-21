#include "effects/render/effect_bass_spin.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <optional>
#include <string>

#include "audio/analyzer.h"
#include "avs/core/RenderContext.hpp"

namespace avs::effects::render {
namespace {
constexpr int kBassWindow = 44;
constexpr double kBaseline = 30.0 * 256.0;
constexpr double kAngleScale = std::numbers::pi_v<double> / 6.0;
constexpr std::array<double, 2> kDirections{{-1.0, 1.0}};

std::array<std::uint8_t, 4> makeColor(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
  return {r, g, b, 255u};
}
}  // namespace

BassSpin::BassSpin() {
  sides_[0].angle = std::numbers::pi_v<double>;
  sides_[1].angle = 0.0;
}

void BassSpin::setParams(const avs::core::ParamBlock& params) {
  if (params.contains("enabled")) {
    enabledMask_ = params.getInt("enabled", enabledMask_);
  }

  auto maskFromBool = [&](const std::string& key, int bit) {
    if (params.contains(key)) {
      if (params.getBool(key, true)) {
        enabledMask_ |= bit;
      } else {
        enabledMask_ &= ~bit;
      }
    }
  };
  maskFromBool("left_enabled", 1);
  maskFromBool("right_enabled", 2);
  maskFromBool("enable_left", 1);
  maskFromBool("enable_right", 2);

  int mode = params.getInt("mode", triangles_ ? 1 : 0);
  if (params.contains("mode")) {
    const std::string modeStr = params.getString("mode", "");
    if (!modeStr.empty()) {
      if (modeStr == "lines" || modeStr == "line") {
        mode = 0;
      } else if (modeStr == "tri" || modeStr == "triangle" || modeStr == "triangles") {
        mode = 1;
      }
    }
  }
  triangles_ = mode != 0;

  colors_[0] = parseColor(params, {"color_left", "color1", "left_color"}, colors_[0]);
  colors_[1] = parseColor(params, {"color_right", "color2", "right_color"}, colors_[1]);
  const Color shared = parseColor(params, {"color", "colour"}, colors_[0]);
  if (params.contains("color") || params.contains("colour")) {
    colors_[0] = shared;
    colors_[1] = shared;
  }
}

bool BassSpin::render(avs::core::RenderContext& context) {
  if (context.width <= 0 || context.height <= 0) {
    return true;
  }
  if (!context.framebuffer.data) {
    return true;
  }
  if (static_cast<std::size_t>(context.width) * static_cast<std::size_t>(context.height) * 4u >
      context.framebuffer.size) {
    return true;
  }

  if (context.width != lastWidth_ || context.height != lastHeight_) {
    resetCachedState(context.width, context.height);
  }

  const int radiusBase = std::min(context.height / 2, (context.width * 3) / 8);
  const int centerOffset = radiusBase / 2;
  const int centerY = context.height / 2;

  for (int side = 0; side < 2; ++side) {
    if ((enabledMask_ & (1 << side)) == 0) {
      continue;
    }

    const double bassSum = computeBassWindow(context);
    const double denominator = lastBassSum_ + kBaseline;
    double a = 0.0;
    if (denominator > 0.0) {
      a = (bassSum * 512.0) / denominator;
    }
    lastBassSum_ = bassSum;
    a = std::clamp(a, 0.0, 255.0);

    const double velocityTarget = std::max(a - 104.0, 12.0) / 96.0;
    auto& state = sides_[static_cast<std::size_t>(side)];
    state.velocity = 0.3 * state.velocity + 0.7 * velocityTarget;
    state.angle += kAngleScale * state.velocity * kDirections[static_cast<std::size_t>(side)];

    const double radius = static_cast<double>(radiusBase) * (a / 256.0);
    const double cosVal = std::cos(state.angle);
    const double sinVal = std::sin(state.angle);
    const int offsetX = static_cast<int>(std::lround(cosVal * radius));
    const int offsetY = static_cast<int>(std::lround(sinVal * radius));

    const int centerX =
        side == 0 ? context.width / 2 - centerOffset : context.width / 2 + centerOffset;
    drawArms(context, centerX, centerY, offsetX, offsetY, state,
             colors_[static_cast<std::size_t>(side)]);
  }

  return true;
}

void BassSpin::resetCachedState(int width, int height) {
  lastWidth_ = width;
  lastHeight_ = height;
  for (auto& side : sides_) {
    for (auto& point : side.lastPoints) {
      point.valid = false;
    }
  }
}

BassSpin::Color BassSpin::parseColor(const avs::core::ParamBlock& params,
                                     const std::initializer_list<std::string>& keys,
                                     Color fallback) {
  for (const auto& key : keys) {
    if (!params.contains(key)) {
      continue;
    }
    const std::string value = params.getString(key, "");
    if (!value.empty()) {
      return parseColorString(value, fallback);
    }
    const int numeric = params.getInt(key, -1);
    if (numeric >= 0) {
      return colorFromInt(numeric, fallback);
    }
  }
  return fallback;
}

BassSpin::Color BassSpin::parseColorString(std::string_view value, Color fallback) {
  std::string cleaned(value);
  if (cleaned.empty()) {
    return fallback;
  }
  if (cleaned.front() == '#') {
    cleaned.erase(cleaned.begin());
  }
  if (cleaned.size() != 6) {
    return fallback;
  }
  auto hexToInt = [](std::string_view slice) -> std::optional<int> {
    int result = 0;
    for (char c : slice) {
      result <<= 4;
      if (c >= '0' && c <= '9') {
        result |= c - '0';
      } else if (c >= 'a' && c <= 'f') {
        result |= 10 + (c - 'a');
      } else if (c >= 'A' && c <= 'F') {
        result |= 10 + (c - 'A');
      } else {
        return std::nullopt;
      }
    }
    return result;
  };
  const auto r = hexToInt(cleaned.substr(0, 2));
  const auto g = hexToInt(cleaned.substr(2, 2));
  const auto b = hexToInt(cleaned.substr(4, 2));
  if (!r || !g || !b) {
    return fallback;
  }
  return makeColor(static_cast<std::uint8_t>(*r), static_cast<std::uint8_t>(*g),
                   static_cast<std::uint8_t>(*b));
}

BassSpin::Color BassSpin::colorFromInt(int rgb, Color fallback) {
  if (rgb < 0) {
    return fallback;
  }
  const std::uint8_t r = static_cast<std::uint8_t>(rgb & 0xFF);
  const std::uint8_t g = static_cast<std::uint8_t>((rgb >> 8) & 0xFF);
  const std::uint8_t b = static_cast<std::uint8_t>((rgb >> 16) & 0xFF);
  return makeColor(r, g, b);
}

void BassSpin::putPixel(avs::core::RenderContext& context, int x, int y, const Color& color) {
  if (x < 0 || y < 0 || x >= context.width || y >= context.height) {
    return;
  }
  const int stride = context.width * 4;
  const int index = y * stride + x * 4;
  if (index < 0 || static_cast<std::size_t>(index + 3) >= context.framebuffer.size) {
    return;
  }
  std::uint8_t* data = context.framebuffer.data + index;
  data[0] = color[0];
  data[1] = color[1];
  data[2] = color[2];
  data[3] = color[3];
}

void BassSpin::drawLine(avs::core::RenderContext& context, int x0, int y0, int x1, int y1,
                        const Color& color) {
  int dx = std::abs(x1 - x0);
  int sx = x0 < x1 ? 1 : -1;
  int dy = -std::abs(y1 - y0);
  int sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  int x = x0;
  int y = y0;
  while (true) {
    putPixel(context, x, y, color);
    if (x == x1 && y == y1) {
      break;
    }
    const int e2 = err * 2;
    if (e2 >= dy) {
      err += dy;
      x += sx;
    }
    if (e2 <= dx) {
      err += dx;
      y += sy;
    }
  }
}

void BassSpin::drawTriangle(avs::core::RenderContext& context, int x0, int y0, int x1, int y1,
                            int x2, int y2, const Color& color) {
  const int minX = std::max(0, std::min({x0, x1, x2}));
  const int maxX = std::min(context.width - 1, std::max({x0, x1, x2}));
  const int minY = std::max(0, std::min({y0, y1, y2}));
  const int maxY = std::min(context.height - 1, std::max({y0, y1, y2}));
  auto edge = [](int ax, int ay, int bx, int by, int px, int py) {
    return (px - ax) * (by - ay) - (py - ay) * (bx - ax);
  };
  const int area = edge(x0, y0, x1, y1, x2, y2);
  if (area == 0) {
    drawLine(context, x0, y0, x1, y1, color);
    drawLine(context, x1, y1, x2, y2, color);
    drawLine(context, x2, y2, x0, y0, color);
    return;
  }
  for (int y = minY; y <= maxY; ++y) {
    for (int x = minX; x <= maxX; ++x) {
      const int w0 = edge(x1, y1, x2, y2, x, y);
      const int w1 = edge(x2, y2, x0, y0, x, y);
      const int w2 = edge(x0, y0, x1, y1, x, y);
      if ((w0 >= 0 && w1 >= 0 && w2 >= 0 && area > 0) ||
          (w0 <= 0 && w1 <= 0 && w2 <= 0 && area < 0)) {
        putPixel(context, x, y, color);
      }
    }
  }
}

double BassSpin::computeBassWindow(const avs::core::RenderContext& context) const {
  const float* data = nullptr;
  std::size_t size = 0;
  if (context.audioSpectrum.data && context.audioSpectrum.size > 0) {
    data = context.audioSpectrum.data;
    size = context.audioSpectrum.size;
  } else if (context.audioAnalysis) {
    data = context.audioAnalysis->spectrum.data();
    size = context.audioAnalysis->spectrum.size();
  }
  if (!data || size == 0) {
    return 0.0;
  }
  const std::size_t count = std::min<std::size_t>(kBassWindow, size);
  double sum = 0.0;
  for (std::size_t i = 0; i < count; ++i) {
    const float magnitude = std::max(data[i], 0.0f);
    const float scaled = std::clamp(magnitude * 255.0f, 0.0f, 255.0f);
    sum += static_cast<double>(scaled);
  }
  return sum;
}

void BassSpin::drawArms(avs::core::RenderContext& context, int centerX, int centerY, int offsetX,
                        int offsetY, SideState& state, const Color& color) {
  const std::array<int, 2> xs{centerX + offsetX, centerX - offsetX};
  const std::array<int, 2> ys{centerY + offsetY, centerY - offsetY};
  for (std::size_t arm = 0; arm < 2; ++arm) {
    ArmPoint& prev = state.lastPoints[arm];
    const int targetX = xs[arm];
    const int targetY = ys[arm];
    if (triangles_) {
      if (prev.valid) {
        drawTriangle(context, centerX, centerY, prev.x, prev.y, targetX, targetY, color);
      }
    } else {
      if (prev.valid) {
        drawLine(context, prev.x, prev.y, targetX, targetY, color);
      }
      drawLine(context, centerX, centerY, targetX, targetY, color);
    }
    prev.x = targetX;
    prev.y = targetY;
    prev.valid = true;
  }
}

}  // namespace avs::effects::render
