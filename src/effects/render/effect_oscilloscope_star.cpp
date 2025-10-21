#include "effects/render/effect_oscilloscope_star.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <numbers>
#include <optional>
#include <string>

#include "audio/analyzer.h"

namespace avs::effects::render {

namespace {

constexpr double kTwoPi = std::numbers::pi * 2.0;

[[nodiscard]] std::string toLowerCopy(const std::string& value) {
  std::string result = value;
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  return result;
}

[[nodiscard]] std::string trimCopy(const std::string& value) {
  std::size_t begin = 0;
  while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin]))) {
    ++begin;
  }
  std::size_t end = value.size();
  while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
    --end;
  }
  return value.substr(begin, end - begin);
}

[[nodiscard]] std::optional<std::uint32_t> parseColorString(const std::string& value) {
  if (value.empty()) {
    return std::nullopt;
  }
  std::string text = trimCopy(value);
  if (text.empty()) {
    return std::nullopt;
  }
  if (text.size() > 2 && (text[0] == '0') && (text[1] == 'x' || text[1] == 'X')) {
    text = text.substr(2);
  }
  if (!text.empty() && text.front() == '#') {
    text = text.substr(1);
  }
  if (text.size() != 6) {
    return std::nullopt;
  }
  std::uint32_t numeric = 0;
  for (char ch : text) {
    if (!std::isxdigit(static_cast<unsigned char>(ch))) {
      return std::nullopt;
    }
    numeric <<= 4;
    if (ch >= '0' && ch <= '9') {
      numeric |= static_cast<std::uint32_t>(ch - '0');
    } else if (ch >= 'a' && ch <= 'f') {
      numeric |= static_cast<std::uint32_t>(10 + (ch - 'a'));
    } else if (ch >= 'A' && ch <= 'F') {
      numeric |= static_cast<std::uint32_t>(10 + (ch - 'A'));
    } else {
      return std::nullopt;
    }
  }
  return numeric;
}

bool hasFramebuffer(const avs::core::RenderContext& context) {
  return context.width > 0 && context.height > 0 && context.framebuffer.data &&
         context.framebuffer.size >= static_cast<std::size_t>(context.width * context.height * 4);
}

}  // namespace

OscilloscopeStar::Color OscilloscopeStar::makeColor(std::uint8_t r, std::uint8_t g, std::uint8_t b,
                                                    std::uint8_t a) {
  return Color{r, g, b, a};
}

OscilloscopeStar::Color OscilloscopeStar::colorFromRgbInt(std::uint32_t rgb) {
  const std::uint8_t r = static_cast<std::uint8_t>((rgb >> 16) & 0xFFu);
  const std::uint8_t g = static_cast<std::uint8_t>((rgb >> 8) & 0xFFu);
  const std::uint8_t b = static_cast<std::uint8_t>(rgb & 0xFFu);
  return makeColor(r, g, b);
}

void OscilloscopeStar::putPixel(avs::core::RenderContext& context, int x, int y,
                                const Color& color) {
  if (x < 0 || y < 0 || x >= context.width || y >= context.height) {
    return;
  }
  const int stride = context.width * 4;
  std::uint8_t* base = context.framebuffer.data;
  const int index = y * stride + x * 4;
  base[index + 0] = color[0];
  base[index + 1] = color[1];
  base[index + 2] = color[2];
  base[index + 3] = color[3];
}

void OscilloscopeStar::drawLine(avs::core::RenderContext& context, int x0, int y0, int x1, int y1,
                                const Color& color) {
  const int dx = std::abs(x1 - x0);
  const int sx = x0 < x1 ? 1 : -1;
  const int dy = -std::abs(y1 - y0);
  const int sy = y0 < y1 ? 1 : -1;
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

std::optional<OscilloscopeStar::Color> OscilloscopeStar::tryParseColor(
    const avs::core::ParamBlock& params, const std::string& key) {
  if (!params.contains(key)) {
    return std::nullopt;
  }
  const std::string text = params.getString(key, "");
  if (!text.empty()) {
    if (auto numeric = parseColorString(text)) {
      return colorFromRgbInt(*numeric);
    }
  }
  const int numeric = params.getInt(key, -1);
  if (numeric >= 0) {
    return colorFromRgbInt(static_cast<std::uint32_t>(numeric));
  }
  return std::nullopt;
}

void OscilloscopeStar::parseColors(const avs::core::ParamBlock& params) {
  std::vector<Color> parsed;
  auto append = [&](const std::string& key) {
    if (auto color = tryParseColor(params, key)) {
      parsed.push_back(*color);
    }
  };
  append("color");
  for (int i = 0; i < 16; ++i) {
    append("color" + std::to_string(i));
  }
  for (int i = 1; i <= 16; ++i) {
    append("color" + std::to_string(i));
  }
  if (!parsed.empty()) {
    palette_ = std::move(parsed);
    const int modulo = static_cast<int>(palette_.size()) * 64;
    if (modulo > 0) {
      colorPhase_ %= modulo;
    } else {
      colorPhase_ = 0;
    }
  }
}

void OscilloscopeStar::advanceColor() {
  if (palette_.empty()) {
    return;
  }
  const int modulo = static_cast<int>(palette_.size()) * 64;
  if (modulo <= 0) {
    return;
  }
  colorPhase_ = (colorPhase_ + 1) % modulo;
}

OscilloscopeStar::Color OscilloscopeStar::currentColor() const {
  if (palette_.empty()) {
    return makeColor(255, 255, 255);
  }
  if (palette_.size() == 1) {
    return palette_.front();
  }
  const int baseIndex = (colorPhase_ / 64) % static_cast<int>(palette_.size());
  const int nextIndex = (baseIndex + 1) % static_cast<int>(palette_.size());
  const float t = static_cast<float>(colorPhase_ % 64) / 64.0f;
  Color result{};
  for (std::size_t i = 0; i < 4; ++i) {
    const float a = static_cast<float>(palette_[baseIndex][i]);
    const float b = static_cast<float>(palette_[nextIndex][i]);
    result[i] = static_cast<std::uint8_t>(std::lround(a * (1.0f - t) + b * t));
  }
  return result;
}

bool OscilloscopeStar::render(avs::core::RenderContext& context) {
  if (!hasFramebuffer(context)) {
    return true;
  }
  if (!context.audioAnalysis) {
    return true;
  }

  const auto& waveform = context.audioAnalysis->waveform;
  if (waveform.empty()) {
    return true;
  }

  advanceColor();
  const Color color = currentColor();

  const double sizeScale = std::clamp(size_, 0, 32) / 32.0;
  const double width = static_cast<double>(context.width);
  const double height = static_cast<double>(context.height);
  const double radius = std::min(width, height) * sizeScale;
  if (radius <= 0.0) {
    return true;
  }

  double centerX = width * 0.5;
  switch (anchor_) {
    case Anchor::Left:
      centerX = width * 0.25;
      break;
    case Anchor::Right:
      centerX = width * 0.75;
      break;
    case Anchor::Center:
    default:
      centerX = width * 0.5;
      break;
  }
  const double centerY = height * 0.5;

  const int totalSteps = kArms * kSegments;
  const std::size_t waveformSize = waveform.size();
  const double sampleStride =
      waveformSize > 0 ? static_cast<double>(waveformSize) / static_cast<double>(totalSteps) : 1.0;
  double sampleCursor = 0.0;

  const double dfStart = 1.0 / 1024.0;
  const double dfEnd = 1.0 / 128.0;
  const double dfStep = (dfEnd - dfStart) / static_cast<double>(std::max(1, kSegments - 1));
  const double radialStep = radius / static_cast<double>(kSegments);

  for (int arm = 0; arm < kArms; ++arm) {
    const double angle =
        rotationAngle_ + static_cast<double>(arm) * (kTwoPi / static_cast<double>(kArms));
    const double s = std::sin(angle);
    const double c = std::cos(angle);
    double radial = 0.0;
    double dfactor = dfStart;
    double prevX = centerX;
    double prevY = centerY;

    for (int segment = 0; segment < kSegments; ++segment) {
      std::size_t sampleIndex = 0;
      if (waveformSize > 0) {
        sampleIndex = static_cast<std::size_t>(
            std::clamp(sampleCursor, 0.0, static_cast<double>(waveformSize - 1)));
      }
      float sample = 0.0f;
      if (sampleIndex < waveformSize) {
        sample = waveform[sampleIndex];
      }
      sampleCursor += sampleStride;

      const double offset = static_cast<double>(sample) * 128.0 * dfactor * radius;
      const double x = centerX + c * radial - s * offset;
      const double y = centerY + s * radial + c * offset;

      drawLine(context, static_cast<int>(std::lround(prevX)), static_cast<int>(std::lround(prevY)),
               static_cast<int>(std::lround(x)), static_cast<int>(std::lround(y)), color);

      prevX = x;
      prevY = y;
      radial += radialStep;
      dfactor += dfStep;
    }
  }

  rotationAngle_ += static_cast<double>(rotationSpeed_) * 0.01;
  if (rotationAngle_ >= kTwoPi) {
    rotationAngle_ = std::fmod(rotationAngle_, kTwoPi);
  } else if (rotationAngle_ <= -kTwoPi) {
    rotationAngle_ = std::fmod(rotationAngle_, kTwoPi);
  }

  return true;
}

void OscilloscopeStar::setParams(const avs::core::ParamBlock& params) {
  size_ = std::clamp(params.getInt("size", size_), 0, 32);
  rotationSpeed_ = std::clamp(params.getFloat("rotation", rotationSpeed_), -64.0f, 64.0f);
  rotationSpeed_ = std::clamp(params.getFloat("rotation_speed", rotationSpeed_), -64.0f, 64.0f);

  if (params.contains("channel")) {
    const std::string channelText = toLowerCopy(params.getString("channel", ""));
    if (!channelText.empty()) {
      if (channelText == "left" || channelText == "l") {
        channel_ = Channel::Left;
      } else if (channelText == "right" || channelText == "r") {
        channel_ = Channel::Right;
      } else {
        channel_ = Channel::Center;
      }
    } else {
      const int numeric = params.getInt("channel", static_cast<int>(channel_));
      if (numeric == 0)
        channel_ = Channel::Left;
      else if (numeric == 1)
        channel_ = Channel::Right;
      else
        channel_ = Channel::Center;
    }
  }

  if (params.contains("position")) {
    const std::string positionText = toLowerCopy(params.getString("position", ""));
    if (!positionText.empty()) {
      if (positionText == "left" || positionText == "l") {
        anchor_ = Anchor::Left;
      } else if (positionText == "right" || positionText == "r") {
        anchor_ = Anchor::Right;
      } else {
        anchor_ = Anchor::Center;
      }
    } else {
      const int numeric = params.getInt("position", static_cast<int>(anchor_));
      if (numeric == 0)
        anchor_ = Anchor::Left;
      else if (numeric == 1)
        anchor_ = Anchor::Right;
      else
        anchor_ = Anchor::Center;
    }
  }

  parseColors(params);
}

}  // namespace avs::effects::render
