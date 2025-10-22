#include <avs/effects/legacy/render/effect_rotating_stars.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <limits>
#include <numbers>
#include <sstream>
#include <string>
#include <string_view>

#include <avs/audio/analyzer.h>
#include <avs/core/RenderContext.hpp>

namespace avs::effects::render {
namespace {

constexpr double kStarStep = std::numbers::pi * 4.0 / 5.0;

std::string sanitizeToken(std::string token) {
  token.erase(std::remove_if(
                  token.begin(), token.end(),
                  [](unsigned char ch) { return ch == ',' || ch == ';' || std::isspace(ch) != 0; }),
              token.end());
  if (!token.empty() && token.front() == '#') {
    token.erase(token.begin());
  }
  if (token.size() > 2 && token[0] == '0' && (token[1] == 'x' || token[1] == 'X')) {
    token.erase(token.begin(), token.begin() + 2);
  }
  return token;
}

bool parseHex(std::string_view token, std::uint32_t& value) {
  if (token.empty()) {
    return false;
  }
  std::stringstream stream;
  stream << std::hex << token;
  stream >> value;
  return !stream.fail();
}

using ParsedRgba = std::array<std::uint8_t, 4>;

std::vector<ParsedRgba> parseColorListString(std::string_view text) {
  std::vector<ParsedRgba> result;
  if (text.empty()) {
    return result;
  }
  std::stringstream stream{std::string(text)};
  std::string token;
  auto applyToken = [&](std::string_view rawToken) {
    std::string tokenValue = sanitizeToken(std::string(rawToken));
    if (tokenValue.empty()) {
      return;
    }
    std::uint32_t parsed = 0;
    if (parseHex(tokenValue, parsed)) {
      ParsedRgba color{0u, 0u, 0u, 255u};
      if (tokenValue.size() > 6u) {
        color[3] = static_cast<std::uint8_t>((parsed >> 24u) & 0xFFu);
      }
      color[0] = static_cast<std::uint8_t>((parsed >> 16u) & 0xFFu);
      color[1] = static_cast<std::uint8_t>((parsed >> 8u) & 0xFFu);
      color[2] = static_cast<std::uint8_t>(parsed & 0xFFu);
      result.push_back(color);
    }
  };

  while (stream >> token) {
    if (token.empty()) {
      continue;
    }
    std::size_t start = 0;
    while (start < token.size()) {
      const std::size_t end = token.find_first_of(",;", start);
      const std::size_t length = (end == std::string::npos) ? token.size() - start : end - start;
      applyToken(std::string_view(token).substr(start, length));
      if (end == std::string::npos) {
        break;
      }
      start = end + 1;
    }
  }
  return result;
}

}  // namespace

RotatingStars::RotatingStars() { palette_.push_back(makeColor(255, 255, 255)); }

RotatingStars::Color RotatingStars::makeColor(std::uint8_t r, std::uint8_t g, std::uint8_t b,
                                              std::uint8_t a) {
  return Color{r, g, b, a};
}

RotatingStars::Color RotatingStars::makeColorFromInt(std::uint32_t packed) {
  const std::uint8_t r = static_cast<std::uint8_t>((packed >> 16u) & 0xFFu);
  const std::uint8_t g = static_cast<std::uint8_t>((packed >> 8u) & 0xFFu);
  const std::uint8_t b = static_cast<std::uint8_t>(packed & 0xFFu);
  return makeColor(r, g, b);
}

void RotatingStars::putPixel(avs::core::RenderContext& context, int x, int y, const Color& color) {
  if (!context.framebuffer.data || x < 0 || y < 0 || x >= context.width || y >= context.height) {
    return;
  }
  const std::size_t index = (static_cast<std::size_t>(y) * static_cast<std::size_t>(context.width) +
                             static_cast<std::size_t>(x)) *
                            4u;
  if (index + 3u >= context.framebuffer.size) {
    return;
  }
  context.framebuffer.data[index + 0u] = color[0];
  context.framebuffer.data[index + 1u] = color[1];
  context.framebuffer.data[index + 2u] = color[2];
  context.framebuffer.data[index + 3u] = color[3];
}

void RotatingStars::drawLine(avs::core::RenderContext& context, int x0, int y0, int x1, int y1,
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

void RotatingStars::updatePalette(const avs::core::ParamBlock& params) {
  std::vector<Color> parsed;
  const std::array<std::string, 2> keys{"colors", "palette"};
  for (const std::string& key : keys) {
    const std::string text = params.getString(key, "");
    if (!text.empty()) {
      auto fromText = parseColorListString(text);
      for (const auto& rgba : fromText) {
        parsed.push_back(makeColor(rgba[0], rgba[1], rgba[2], rgba[3]));
      }
    }
  }
  for (int i = 0; i < kMaxPaletteSize; ++i) {
    const std::string key = "color" + std::to_string(i);
    if (params.contains(key)) {
      parsed.push_back(makeColorFromInt(static_cast<std::uint32_t>(params.getInt(key, 0))));
    }
  }
  if (parsed.empty() && params.contains("color")) {
    parsed.push_back(makeColorFromInt(static_cast<std::uint32_t>(params.getInt("color", 0))));
  }

  if (!parsed.empty()) {
    if (parsed.size() > static_cast<std::size_t>(kMaxPaletteSize)) {
      parsed.resize(static_cast<std::size_t>(kMaxPaletteSize));
    }
    palette_ = std::move(parsed);
    if (!palette_.empty()) {
      const int cycleLength = static_cast<int>(palette_.size()) * kColorCycleLength;
      if (cycleLength > 0) {
        colorPos_ %= cycleLength;
      } else {
        colorPos_ = 0;
      }
    }
  } else if (palette_.empty()) {
    palette_.push_back(makeColor(255, 255, 255));
    colorPos_ = 0;
  }
}

RotatingStars::Color RotatingStars::currentColor() {
  if (palette_.empty()) {
    return makeColor(255, 255, 255);
  }

  if (palette_.size() == 1u) {
    colorPos_ = (colorPos_ + 1) % kColorCycleLength;
    Color color = palette_.front();
    color[3] = 255u;
    return color;
  }

  const int totalSteps = static_cast<int>(palette_.size()) * kColorCycleLength;
  if (totalSteps <= 0) {
    return palette_.front();
  }

  colorPos_ = (colorPos_ + 1) % totalSteps;
  const int index = colorPos_ / kColorCycleLength;
  const int frac = colorPos_ % kColorCycleLength;
  const Color& a = palette_[index];
  const Color& b = palette_[(index + 1) % palette_.size()];

  Color result{};
  for (std::size_t i = 0; i < 3; ++i) {
    const int blended =
        (static_cast<int>(a[i]) * (kColorCycleLength - 1 - frac) + static_cast<int>(b[i]) * frac) /
        kColorCycleLength;
    result[i] = static_cast<std::uint8_t>(std::clamp(blended, 0, 255));
  }
  result[3] = 255u;
  return result;
}

std::array<float, 2> RotatingStars::computeChannelAmplitudes(
    const avs::core::RenderContext& context) const {
  const float* spectrumPtr = nullptr;
  std::size_t spectrumSize = 0;
  if (context.audioSpectrum.data && context.audioSpectrum.size > 0) {
    spectrumPtr = context.audioSpectrum.data;
    spectrumSize = context.audioSpectrum.size;
  } else if (context.audioAnalysis) {
    spectrumPtr = context.audioAnalysis->spectrum.data();
    spectrumSize = context.audioAnalysis->spectrum.size();
  }

  const float peak = computeSpectrumPeak(spectrumPtr, spectrumSize);
  return {peak, peak};
}

float RotatingStars::computeSpectrumPeak(const float* spectrum, std::size_t size) const {
  if (!spectrum || size == 0) {
    return 0.0f;
  }
  float maxMagnitude = 0.0f;
  for (std::size_t i = 0; i < size; ++i) {
    maxMagnitude = std::max(maxMagnitude, spectrum[i]);
  }
  if (maxMagnitude <= std::numeric_limits<float>::epsilon()) {
    return 0.0f;
  }

  auto sample = [&](int band) {
    if (size == 1) {
      return spectrum[0];
    }
    const float position = static_cast<float>(band) * static_cast<float>(size - 1) / 575.0f;
    const std::size_t base = static_cast<std::size_t>(std::floor(position));
    const std::size_t next = std::min(base + 1, size - 1);
    const float frac = position - static_cast<float>(base);
    const float value = spectrum[base] * (1.0f - frac) + spectrum[next] * frac;
    return value;
  };

  const float scale = 255.0f / maxMagnitude;
  float best = 0.0f;
  for (int band = 3; band < 14; ++band) {
    const float current = sample(band) * scale;
    const float prev = sample(band - 1) * scale;
    const float next = sample(band + 1) * scale;
    if (current > best && current > prev + 4.0f && current > next + 4.0f) {
      best = current;
    }
  }
  return best;
}

bool RotatingStars::render(avs::core::RenderContext& context) {
  if (context.width <= 0 || context.height <= 0) {
    return true;
  }
  if (!context.framebuffer.data) {
    return true;
  }
  const std::size_t requiredSize =
      static_cast<std::size_t>(context.width) * static_cast<std::size_t>(context.height) * 4u;
  if (context.framebuffer.size < requiredSize) {
    return true;
  }

  const Color color = currentColor();
  const auto amplitudes = computeChannelAmplitudes(context);

  const double centerX = static_cast<double>(context.width) * 0.5;
  const double centerY = static_cast<double>(context.height) * 0.5;
  const double offsetX = std::cos(rotation_) * static_cast<double>(context.width) * 0.25;
  const double offsetY = std::sin(rotation_) * static_cast<double>(context.height) * 0.25;

  for (int channel = 0; channel < 2; ++channel) {
    const double amplitude = static_cast<double>(amplitudes[static_cast<std::size_t>(channel)]);
    const double scaleX = static_cast<double>(context.width) / 8.0 * (amplitude + 9.0) / 88.0;
    const double scaleY = static_cast<double>(context.height) / 8.0 * (amplitude + 9.0) / 88.0;

    const double baseX = centerX + (channel == 0 ? offsetX : -offsetX);
    const double baseY = centerY + (channel == 0 ? offsetY : -offsetY);

    double angle = -rotation_;
    int prevX = static_cast<int>(std::lround(baseX + std::cos(angle) * scaleX));
    int prevY = static_cast<int>(std::lround(baseY + std::sin(angle) * scaleY));
    angle += kStarStep;
    for (int point = 0; point < 5; ++point) {
      const int nextX = static_cast<int>(std::lround(baseX + std::cos(angle) * scaleX));
      const int nextY = static_cast<int>(std::lround(baseY + std::sin(angle) * scaleY));
      drawLine(context, prevX, prevY, nextX, nextY, color);
      prevX = nextX;
      prevY = nextY;
      angle += kStarStep;
    }
  }

  rotation_ += rotationSpeed_;
  return true;
}

void RotatingStars::setParams(const avs::core::ParamBlock& params) {
  updatePalette(params);
  if (params.contains("speed")) {
    rotationSpeed_ =
        static_cast<double>(params.getFloat("speed", static_cast<float>(rotationSpeed_)));
  }
  if (params.contains("rotation_speed")) {
    rotationSpeed_ =
        static_cast<double>(params.getFloat("rotation_speed", static_cast<float>(rotationSpeed_)));
  }
}

}  // namespace avs::effects::render
