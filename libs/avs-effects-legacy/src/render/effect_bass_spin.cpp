#include <avs/effects/render/effect_bass_spin.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numbers>
#include <string>
#include <string_view>

#include <avs/audio/analyzer.h>

namespace avs::effects::render {
namespace {

constexpr double kRotationStep = std::numbers::pi / 6.0;
constexpr float kAmplitudeClamp = 255.0f;
constexpr float kSmoothingBase = 30.0f * 256.0f;

std::string toLowerCopy(std::string_view value) {
  std::string result(value);
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  return result;
}

}  // namespace

BassSpin::BassSpin() {
  colors_[0] = colorFromInt(0xFFFFFFu);
  colors_[1] = colorFromInt(0xFFFFFFu);
  angles_[0] = std::numbers::pi;
  angles_[1] = 0.0;
  velocities_.fill(0.0);
  directions_[0] = -1.0;
  directions_[1] = 1.0;
  clearTrails();
}

bool BassSpin::hasFramebuffer(const avs::core::RenderContext& context) {
  if (!context.framebuffer.data || context.width <= 0 || context.height <= 0) {
    return false;
  }
  const std::size_t required =
      static_cast<std::size_t>(context.width) * static_cast<std::size_t>(context.height) * 4u;
  return context.framebuffer.size >= required;
}

BassSpin::Color BassSpin::colorFromInt(std::uint32_t value) {
  Color color{};
  color.r = static_cast<std::uint8_t>((value >> 16u) & 0xFFu);
  color.g = static_cast<std::uint8_t>((value >> 8u) & 0xFFu);
  color.b = static_cast<std::uint8_t>(value & 0xFFu);
  color.a = 255u;
  return color;
}

bool BassSpin::parseColorToken(std::string_view token, Color& color) {
  if (token.empty()) {
    return false;
  }
  while (!token.empty() && std::isspace(static_cast<unsigned char>(token.front()))) {
    token.remove_prefix(1);
  }
  while (!token.empty() && std::isspace(static_cast<unsigned char>(token.back()))) {
    token.remove_suffix(1);
  }
  if (token.empty()) {
    return false;
  }
  int base = 10;
  if (token.front() == '#') {
    token.remove_prefix(1);
    base = 16;
  } else if (token.size() > 2 && token.front() == '0' && (token[1] == 'x' || token[1] == 'X')) {
    token.remove_prefix(2);
    base = 16;
  } else {
    bool allHex = true;
    for (char ch : token) {
      if (!std::isxdigit(static_cast<unsigned char>(ch))) {
        allHex = false;
        break;
      }
    }
    if (allHex) {
      base = 16;
    }
  }
  std::uint32_t value = 0;
  auto result = std::from_chars(token.data(), token.data() + token.size(), value, base);
  if (result.ec != std::errc() || result.ptr != token.data() + token.size()) {
    return false;
  }
  color = colorFromInt(value);
  return true;
}

void BassSpin::writePixel(avs::core::RenderContext& context, int x, int y, const Color& color) {
  if (x < 0 || y < 0 || x >= context.width || y >= context.height) {
    return;
  }
  const std::size_t index = (static_cast<std::size_t>(y) * static_cast<std::size_t>(context.width) +
                             static_cast<std::size_t>(x)) *
                            4u;
  if (index + 3u >= context.framebuffer.size) {
    return;
  }
  std::uint8_t* pixel = context.framebuffer.data + index;
  pixel[0] = color.r;
  pixel[1] = color.g;
  pixel[2] = color.b;
  pixel[3] = color.a;
}

void BassSpin::drawLine(avs::core::RenderContext& context, int x0, int y0, int x1, int y1,
                        const Color& color) {
  int dx = std::abs(x1 - x0);
  int sx = x0 < x1 ? 1 : -1;
  int dy = -std::abs(y1 - y0);
  int sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  while (true) {
    writePixel(context, x0, y0, color);
    if (x0 == x1 && y0 == y1) {
      break;
    }
    const int twiceErr = err * 2;
    if (twiceErr >= dy) {
      err += dy;
      x0 += sx;
    }
    if (twiceErr <= dx) {
      err += dx;
      y0 += sy;
    }
  }
}

void BassSpin::drawTriangle(avs::core::RenderContext& context, int x0, int y0, int x1, int y1,
                            int x2, int y2, const Color& color) {
  int minX = std::min({x0, x1, x2});
  int maxX = std::max({x0, x1, x2});
  int minY = std::min({y0, y1, y2});
  int maxY = std::max({y0, y1, y2});

  minX = std::max(minX, 0);
  minY = std::max(minY, 0);
  maxX = std::min(maxX, context.width - 1);
  maxY = std::min(maxY, context.height - 1);
  if (minX > maxX || minY > maxY) {
    return;
  }

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
  const bool positive = area > 0;

  for (int y = minY; y <= maxY; ++y) {
    for (int x = minX; x <= maxX; ++x) {
      const int w0 = edge(x1, y1, x2, y2, x, y);
      const int w1 = edge(x2, y2, x0, y0, x, y);
      const int w2 = edge(x0, y0, x1, y1, x, y);
      if (positive) {
        if (w0 < 0 || w1 < 0 || w2 < 0) {
          continue;
        }
      } else {
        if (w0 > 0 || w1 > 0 || w2 > 0) {
          continue;
        }
      }
      writePixel(context, x, y, color);
    }
  }
}

void BassSpin::parseEnabledMask(const avs::core::ParamBlock& params) {
  int mask = params.getInt("enabled", enabledMask_);
  mask &= 0b11;

  auto evaluateToggle = [&params](std::string_view key, bool current) {
    const std::string keyString(key);
    if (!params.contains(keyString)) {
      return current;
    }
    const int sentinel = std::numeric_limits<int>::min();
    const int asInt = params.getInt(keyString, sentinel);
    if (asInt != sentinel) {
      return asInt != 0;
    }
    const bool asTrue = params.getBool(keyString, true);
    const bool asFalse = params.getBool(keyString, false);
    if (asTrue == asFalse) {
      return asTrue;
    }
    return current;
  };

  bool leftFlag = evaluateToggle("enable_left", (mask & 0b01) != 0);
  bool rightFlag = evaluateToggle("enable_right", (mask & 0b10) != 0);

  int newMask = 0;
  if (leftFlag) {
    newMask |= 0b01;
  }
  if (rightFlag) {
    newMask |= 0b10;
  }
  if (newMask != enabledMask_) {
    enabledMask_ = newMask;
    clearTrails();
  }
}

void BassSpin::parseMode(const avs::core::ParamBlock& params) {
  if (!params.contains("mode")) {
    return;
  }
  Mode newMode = mode_;
  const std::string modeString = toLowerCopy(params.getString("mode", ""));
  if (!modeString.empty()) {
    if (modeString == "line" || modeString == "lines") {
      newMode = Mode::Lines;
    } else if (modeString == "triangle" || modeString == "triangles" || modeString == "tri") {
      newMode = Mode::Triangles;
    }
  } else {
    const int modeValue = params.getInt("mode", static_cast<int>(mode_ == Mode::Triangles));
    newMode = modeValue == 0 ? Mode::Lines : Mode::Triangles;
  }
  if (newMode != mode_) {
    mode_ = newMode;
    clearTrails();
  }
}

void BassSpin::parseColors(const avs::core::ParamBlock& params) {
  bool leftSet = false;
  bool rightSet = false;

  if (params.contains("color0")) {
    colors_[0] = colorFromInt(static_cast<std::uint32_t>(params.getInt("color0", 0)));
    leftSet = true;
  }
  if (params.contains("color1")) {
    colors_[1] = colorFromInt(static_cast<std::uint32_t>(params.getInt("color1", 0)));
    rightSet = true;
  }
  if (params.contains("color_left")) {
    colors_[0] = colorFromInt(static_cast<std::uint32_t>(params.getInt("color_left", 0)));
    leftSet = true;
  }
  if (params.contains("color_right")) {
    colors_[1] = colorFromInt(static_cast<std::uint32_t>(params.getInt("color_right", 0)));
    rightSet = true;
  }
  if (params.contains("left_color")) {
    colors_[0] = colorFromInt(static_cast<std::uint32_t>(params.getInt("left_color", 0)));
    leftSet = true;
  }
  if (params.contains("right_color")) {
    colors_[1] = colorFromInt(static_cast<std::uint32_t>(params.getInt("right_color", 0)));
    rightSet = true;
  }

  if (!leftSet || !rightSet) {
    if (params.contains("colors")) {
      const std::string raw = params.getString("colors", "");
      std::string token;
      Color parsed{};
      std::string current;
      for (char ch : raw) {
        if (ch == ',' || ch == ';' || std::isspace(static_cast<unsigned char>(ch))) {
          if (!current.empty() && parseColorToken(current, parsed)) {
            if (!leftSet) {
              colors_[0] = parsed;
              leftSet = true;
            } else if (!rightSet) {
              colors_[1] = parsed;
              rightSet = true;
            }
          }
          current.clear();
        } else {
          current.push_back(ch);
        }
      }
      if (!current.empty() && parseColorToken(current, parsed)) {
        if (!leftSet) {
          colors_[0] = parsed;
          leftSet = true;
        } else if (!rightSet) {
          colors_[1] = parsed;
          rightSet = true;
        }
      }
    }
  }

  if (!leftSet && params.contains("color")) {
    colors_[0] = colorFromInt(static_cast<std::uint32_t>(params.getInt("color", 0)));
    leftSet = true;
  }
  if (!rightSet && leftSet) {
    colors_[1] = colors_[0];
    rightSet = true;
  }
  if (!leftSet && rightSet) {
    colors_[0] = colors_[1];
  }
}

void BassSpin::clearTrails() {
  for (auto& channel : trails_) {
    for (auto& trail : channel) {
      trail.valid = false;
      trail.x = 0;
      trail.y = 0;
    }
  }
}

float BassSpin::computeBassWindow(const avs::audio::Analysis& analysis) const {
  const std::size_t count = std::min(kBassWindow, analysis.waveform.size());
  float sum = 0.0f;
  for (std::size_t i = 0; i < count; ++i) {
    const float normalized = std::clamp((analysis.waveform[i] + 1.0f) * 0.5f, 0.0f, 1.0f);
    sum += normalized * 255.0f;
  }
  return sum;
}

float BassSpin::computeAmplitude(float bassSum) {
  float denominator = lastBassSum_ + kSmoothingBase;
  if (denominator < std::numeric_limits<float>::epsilon()) {
    denominator = kSmoothingBase;
  }
  float amplitude = (bassSum * 512.0f) / denominator;
  if (amplitude > kAmplitudeClamp) {
    amplitude = kAmplitudeClamp;
  }
  lastBassSum_ = bassSum;
  return amplitude;
}

void BassSpin::setParams(const avs::core::ParamBlock& params) {
  parseEnabledMask(params);
  parseMode(params);
  parseColors(params);
}

bool BassSpin::render(avs::core::RenderContext& context) {
  if (!hasFramebuffer(context)) {
    return true;
  }
  const int ss = std::min(context.height / 2, (context.width * 3) / 8);
  if (ss <= 0) {
    return true;
  }

  const avs::audio::Analysis* analysis = context.audioAnalysis;
  const float bassSum = analysis ? computeBassWindow(*analysis) : 0.0f;

  for (int channel = 0; channel < kChannelCount; ++channel) {
    if ((enabledMask_ & (1 << channel)) == 0) {
      trails_[channel][0].valid = false;
      trails_[channel][1].valid = false;
      continue;
    }

    const float amplitude = computeAmplitude(bassSum);
    const double target = static_cast<double>(std::max(amplitude - 104.0f, 12.0f) / 96.0f);
    velocities_[channel] = 0.7 * target + 0.3 * velocities_[channel];
    angles_[channel] += kRotationStep * velocities_[channel] * directions_[channel];

    const double radius = static_cast<double>(ss) * static_cast<double>(amplitude) / 256.0;
    const int offsetX = static_cast<int>(std::trunc(std::cos(angles_[channel]) * radius));
    const int offsetY = static_cast<int>(std::trunc(std::sin(angles_[channel]) * radius));

    const int centerX = context.width / 2 + (channel == 0 ? -ss / 2 : ss / 2);
    const int centerY = context.height / 2;

    const int primaryX = centerX + offsetX;
    const int primaryY = centerY + offsetY;
    const int mirrorX = centerX - offsetX;
    const int mirrorY = centerY - offsetY;
    Color color = colors_[static_cast<std::size_t>(channel)];

    if (mode_ == Mode::Lines) {
      if (trails_[channel][0].valid) {
        drawLine(context, trails_[channel][0].x, trails_[channel][0].y, primaryX, primaryY, color);
      }
      drawLine(context, centerX, centerY, primaryX, primaryY, color);
      trails_[channel][0] = {primaryX, primaryY, true};

      if (trails_[channel][1].valid) {
        drawLine(context, trails_[channel][1].x, trails_[channel][1].y, mirrorX, mirrorY, color);
      }
      drawLine(context, centerX, centerY, mirrorX, mirrorY, color);
      trails_[channel][1] = {mirrorX, mirrorY, true};
    } else {
      if (trails_[channel][0].valid) {
        drawTriangle(context, centerX, centerY, trails_[channel][0].x, trails_[channel][0].y,
                     primaryX, primaryY, color);
      }
      trails_[channel][0] = {primaryX, primaryY, true};

      if (trails_[channel][1].valid) {
        drawTriangle(context, centerX, centerY, trails_[channel][1].x, trails_[channel][1].y,
                     mirrorX, mirrorY, color);
      }
      trails_[channel][1] = {mirrorX, mirrorY, true};
    }
  }

  return true;
}

}  // namespace avs::effects::render
