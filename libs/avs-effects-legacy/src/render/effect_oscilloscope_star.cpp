#include <avs/effects/render/effect_oscilloscope_star.h>

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

namespace {

std::string sanitizeToken(std::string token) {
  token.erase(std::remove_if(token.begin(), token.end(), [](unsigned char ch) {
                 return ch == ',' || ch == ';' || std::isspace(ch) != 0;
               }),
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

std::vector<std::array<std::uint8_t, 4>> parseColorListString(std::string_view text) {
  std::vector<std::array<std::uint8_t, 4>> result;
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
      std::array<std::uint8_t, 4> color{0u, 0u, 0u, 255u};
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

std::string toLowerCopy(std::string_view value) {
  std::string result(value);
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  return result;
}

}  // namespace

namespace avs::effects::render {

OscilloscopeStar::OscilloscopeStar() { palette_.push_back(makeColor(255, 255, 255)); }

OscilloscopeStar::Color OscilloscopeStar::makeColor(std::uint8_t r,
                                                    std::uint8_t g,
                                                    std::uint8_t b,
                                                    std::uint8_t a) {
  return Color{r, g, b, a};
}

OscilloscopeStar::Color OscilloscopeStar::makeColorFromInt(std::uint32_t packed) {
  const std::uint8_t r = static_cast<std::uint8_t>((packed >> 16u) & 0xFFu);
  const std::uint8_t g = static_cast<std::uint8_t>((packed >> 8u) & 0xFFu);
  const std::uint8_t b = static_cast<std::uint8_t>(packed & 0xFFu);
  return makeColor(r, g, b);
}

void OscilloscopeStar::putPixel(avs::core::RenderContext& context, int x, int y, const Color& color) {
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

void OscilloscopeStar::drawLine(avs::core::RenderContext& context,
                                int x0,
                                int y0,
                                int x1,
                                int y1,
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

void OscilloscopeStar::updatePalette(const avs::core::ParamBlock& params) {
  std::vector<Color> parsed;
  const std::array<std::string, 2> listKeys{"colors", "palette"};
  for (const std::string& key : listKeys) {
    const std::string text = params.getString(key, "");
    if (!text.empty()) {
      auto values = parseColorListString(text);
      for (const auto& rgba : values) {
        parsed.push_back(makeColor(rgba[0], rgba[1], rgba[2], rgba[3]));
      }
    }
  }

  int requestedCount = -1;
  if (params.contains("num_colors")) {
    requestedCount = params.getInt("num_colors", 0);
    if (requestedCount < 0) {
      requestedCount = -1;
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

  if (requestedCount >= 0 && requestedCount < static_cast<int>(parsed.size())) {
    parsed.resize(static_cast<std::size_t>(requestedCount));
  }

  if (!parsed.empty()) {
    if (parsed.size() > static_cast<std::size_t>(kMaxPaletteSize)) {
      parsed.resize(static_cast<std::size_t>(kMaxPaletteSize));
    }
    palette_ = std::move(parsed);
    if (palette_.empty()) {
      palette_.push_back(makeColor(255, 255, 255));
    }
    const int cycleLength = static_cast<int>(palette_.size()) * kColorCycleLength;
    if (cycleLength > 0) {
      colorPos_ %= cycleLength;
    } else {
      colorPos_ = 0;
    }
  } else if (palette_.empty()) {
    palette_.push_back(makeColor(255, 255, 255));
    colorPos_ = 0;
  }
}

void OscilloscopeStar::applyEffectBits(int effect) {
  const int channelBits = (effect >> 2) & 0x3;
  switch (channelBits) {
    case 1:
      channel_ = ChannelMode::Right;
      break;
    case 2:
      channel_ = ChannelMode::Center;
      break;
    default:
      channel_ = ChannelMode::Left;
      break;
  }

  const int anchorBits = (effect >> 4) & 0x3;
  switch (anchorBits) {
    case 0:
      anchor_ = AnchorMode::Left;
      break;
    case 1:
      anchor_ = AnchorMode::Right;
      break;
    case 2:
    default:
      anchor_ = AnchorMode::Center;
      break;
  }
}

void OscilloscopeStar::setChannelFromString(const std::string& value) {
  const std::string lowered = toLowerCopy(value);
  if (lowered == "left" || lowered == "l") {
    channel_ = ChannelMode::Left;
  } else if (lowered == "right" || lowered == "r") {
    channel_ = ChannelMode::Right;
  } else if (lowered == "center" || lowered == "centre" || lowered == "mix" || lowered == "mid") {
    channel_ = ChannelMode::Center;
  }
}

void OscilloscopeStar::setAnchorFromString(const std::string& value) {
  const std::string lowered = toLowerCopy(value);
  if (lowered == "left" || lowered == "l" || lowered == "top") {
    anchor_ = AnchorMode::Left;
  } else if (lowered == "right" || lowered == "r" || lowered == "bottom") {
    anchor_ = AnchorMode::Right;
  } else if (lowered == "center" || lowered == "centre" || lowered == "middle") {
    anchor_ = AnchorMode::Center;
  }
}

void OscilloscopeStar::updateRotationSpeedFromLegacy(float legacyRot) {
  rotationSpeed_ = static_cast<double>(legacyRot) * 0.01;
}

OscilloscopeStar::Color OscilloscopeStar::currentColor() {
  if (palette_.empty()) {
    palette_.push_back(makeColor(255, 255, 255));
    colorPos_ = 0;
    return palette_.front();
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
  const Color& a = palette_[static_cast<std::size_t>(index)];
  const Color& b = palette_[(static_cast<std::size_t>(index) + 1u) % palette_.size()];

  Color result{};
  for (std::size_t i = 0; i < 3; ++i) {
    const int blended =
        (static_cast<int>(a[i]) * (kColorCycleLength - frac) + static_cast<int>(b[i]) * frac) /
        kColorCycleLength;
    result[i] = static_cast<std::uint8_t>(std::clamp(blended, 0, 255));
  }
  result[3] = 255u;
  return result;
}

std::array<float, OscilloscopeStar::kWaveformSamples> OscilloscopeStar::sampleWaveform(
    const avs::core::RenderContext& context) const {
  std::array<float, kWaveformSamples> samples{};
  samples.fill(0.0f);

  std::array<float, kWaveformSamples> base{};
  base.fill(0.0f);
  bool hasData = false;

  if (context.audioAnalysis) {
    const auto& waveform = context.audioAnalysis->waveform;
    const std::size_t copyCount = std::min<std::size_t>(kWaveformSamples, waveform.size());
    std::copy_n(waveform.begin(), copyCount, base.begin());
    hasData = true;
  }

  if (!hasData && context.audioSpectrum.data && context.audioSpectrum.size > 0) {
    const float* spectrum = context.audioSpectrum.data;
    const std::size_t bins = context.audioSpectrum.size;
    if (bins > 0) {
      for (std::size_t i = 0; i < kWaveformSamples; ++i) {
        const double position = static_cast<double>(i) / static_cast<double>(kWaveformSamples - 1);
        const double index = position * static_cast<double>(bins - 1);
        const std::size_t baseIndex = static_cast<std::size_t>(std::floor(index));
        const std::size_t nextIndex = std::min(baseIndex + 1, bins - 1);
        const double frac = index - static_cast<double>(baseIndex);
        const double magnitude = static_cast<double>(spectrum[baseIndex]) * (1.0 - frac) +
                                 static_cast<double>(spectrum[nextIndex]) * frac;
        base[i] = static_cast<float>(std::clamp(magnitude / 255.0, -1.0, 1.0));
      }
      hasData = true;
    }
  }

  if (!hasData) {
    for (std::size_t i = 0; i < kWaveformSamples; ++i) {
      const double t = static_cast<double>(i) / static_cast<double>(kWaveformSamples);
      base[i] = static_cast<float>(std::sin(t * std::numbers::pi));
    }
  }

  switch (channel_) {
    case ChannelMode::Left:
      samples = base;
      break;
    case ChannelMode::Right:
      for (std::size_t i = 0; i < kWaveformSamples; ++i) {
        samples[i] = -base[kWaveformSamples - 1u - i];
      }
      break;
    case ChannelMode::Center:
      for (std::size_t i = 0; i < kWaveformSamples; ++i) {
        samples[i] = 0.5f * (base[i] - base[kWaveformSamples - 1u - i]);
      }
      break;
  }

  return samples;
}

float OscilloscopeStar::interpolateSample(const std::array<float, kWaveformSamples>& samples,
                                          double position) {
  if (samples.empty()) {
    return 0.0f;
  }
  if (position <= 0.0) {
    return samples.front();
  }
  const double maxIndex = static_cast<double>(samples.size() - 1u);
  if (position >= maxIndex) {
    return samples.back();
  }
  const auto baseIndex = static_cast<std::size_t>(position);
  const auto nextIndex = std::min<std::size_t>(baseIndex + 1u, samples.size() - 1u);
  const double frac = position - static_cast<double>(baseIndex);
  return static_cast<float>(samples[baseIndex] * (1.0 - frac) + samples[nextIndex] * frac);
}

double OscilloscopeStar::anchorX(const avs::core::RenderContext& context) const {
  const double width = static_cast<double>(std::max(context.width, 0));
  switch (anchor_) {
    case AnchorMode::Left:
      return width * 0.25;
    case AnchorMode::Right:
      return width * 0.75;
    case AnchorMode::Center:
    default:
      return width * 0.5;
  }
}

bool OscilloscopeStar::render(avs::core::RenderContext& context) {
  if (context.width <= 0 || context.height <= 0) {
    rotation_ += rotationSpeed_;
    return true;
  }
  if (!context.framebuffer.data) {
    rotation_ += rotationSpeed_;
    return true;
  }
  const std::size_t requiredSize = static_cast<std::size_t>(context.width) *
                                   static_cast<std::size_t>(context.height) * 4u;
  if (context.framebuffer.size < requiredSize) {
    rotation_ += rotationSpeed_;
    return true;
  }

  const Color color = currentColor();
  const auto waveform = sampleWaveform(context);

  const double normalizedSize = static_cast<double>(std::max(sizeParam_, 0)) / 32.0;
  const double baseX = anchorX(context);
  const double baseY = static_cast<double>(context.height) * 0.5;
  const double radiusLimit = normalizedSize *
                              static_cast<double>(std::min(context.width, context.height));
  if (radiusLimit <= std::numeric_limits<double>::epsilon()) {
    rotation_ += rotationSpeed_;
    return true;
  }

  const int totalSegments = kArmCount * kSegmentsPerArm;
  const double sampleScale = static_cast<double>(kWaveformSamples - 1u) /
                             static_cast<double>(std::max(totalSegments - 1, 1));
  const double angleStep = 2.0 * std::numbers::pi / static_cast<double>(kArmCount);
  const double dp = radiusLimit / static_cast<double>(kSegmentsPerArm);
  const double dfactorIncrement = (1.0 / 128.0 - 1.0 / 1024.0) / static_cast<double>(kSegmentsPerArm);

  for (int arm = 0; arm < kArmCount; ++arm) {
    const double angle = rotation_ + static_cast<double>(arm) * angleStep;
    const double sinAngle = std::sin(angle);
    const double cosAngle = std::cos(angle);

    double radial = 0.0;
    double dfactor = 1.0 / 1024.0;
    int prevX = static_cast<int>(std::lround(baseX));
    int prevY = static_cast<int>(std::lround(baseY));

    for (int segment = 0; segment < kSegmentsPerArm; ++segment) {
      const int segmentIndex = arm * kSegmentsPerArm + segment;
      const double sampleIndex = static_cast<double>(segmentIndex) * sampleScale;
      const double sampleValue = static_cast<double>(interpolateSample(waveform, sampleIndex));
      const double amplitude = std::clamp(sampleValue, -1.0, 1.0) * 128.0;
      const double offset = amplitude * dfactor * radiusLimit;

      const double nextX = baseX + cosAngle * radial - sinAngle * offset;
      const double nextY = baseY + sinAngle * radial + cosAngle * offset;

      const int xi = static_cast<int>(std::lround(nextX));
      const int yi = static_cast<int>(std::lround(nextY));
      drawLine(context, prevX, prevY, xi, yi, color);

      prevX = xi;
      prevY = yi;
      radial += dp;
      dfactor += dfactorIncrement;
    }
  }

  rotation_ += rotationSpeed_;
  return true;
}

void OscilloscopeStar::setParams(const avs::core::ParamBlock& params) {
  updatePalette(params);

  if (params.contains("size")) {
    sizeParam_ = std::max(0, params.getInt("size", sizeParam_));
  } else if (params.contains("radius")) {
    sizeParam_ = std::max(0, params.getInt("radius", sizeParam_));
  } else if (params.contains("scale")) {
    sizeParam_ = std::max(0, static_cast<int>(std::lround(params.getFloat("scale", static_cast<float>(sizeParam_)))));
  }

  if (params.contains("rot")) {
    updateRotationSpeedFromLegacy(params.getFloat("rot", static_cast<float>(rotationSpeed_ / 0.01)));
  }
  if (params.contains("rotation_speed")) {
    rotationSpeed_ = params.getFloat("rotation_speed", static_cast<float>(rotationSpeed_));
  }
  if (params.contains("speed")) {
    rotationSpeed_ = params.getFloat("speed", static_cast<float>(rotationSpeed_));
  }

  if (params.contains("effect")) {
    applyEffectBits(params.getInt("effect", 0));
  }
  if (params.contains("channel")) {
    setChannelFromString(params.getString("channel", ""));
  }
  if (params.contains("source")) {
    setChannelFromString(params.getString("source", ""));
  }
  if (params.contains("anchor")) {
    setAnchorFromString(params.getString("anchor", ""));
  }
  if (params.contains("position")) {
    setAnchorFromString(params.getString("position", ""));
  }
}

}  // namespace avs::effects::render
