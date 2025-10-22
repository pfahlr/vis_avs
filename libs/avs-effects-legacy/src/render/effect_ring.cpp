#include <avs/effects/render/effect_ring.h>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <limits>

#include <avs/audio/analyzer.h>

namespace avs::effects::render {

namespace {
constexpr float kTwoPi = 6.28318530717958647692f;

Ring::Channel parseChannelFromInt(int value, Ring::Channel fallback) {
  switch (value & 3) {
    case 0:
      return Ring::Channel::Left;
    case 1:
      return Ring::Channel::Right;
    case 2:
      return Ring::Channel::Mix;
    default:
      return fallback;
  }
}

Ring::Placement parsePlacementFromInt(int value, Ring::Placement fallback) {
  switch (value & 3) {
    case 0:
      return Ring::Placement::Left;
    case 1:
      return Ring::Placement::Right;
    case 2:
      return Ring::Placement::Center;
    default:
      return fallback;
  }
}

Ring::Source parseSourceFromInt(int value, Ring::Source fallback) {
  return value == 1 ? Ring::Source::Spectrum : (value == 0 ? Ring::Source::Oscilloscope : fallback);
}

}  // namespace

Ring::Ring() { colors_.push_back(Color{255, 255, 255}); }

int Ring::clampSize(int value) { return std::clamp(value, 1, 64); }

std::string Ring::toLower(std::string_view value) {
  std::string lowered(value);
  std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return lowered;
}

bool Ring::parseColorToken(std::string_view token, Color& color) {
  auto trimmed = token;
  while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.front()))) {
    trimmed.remove_prefix(1);
  }
  while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.back()))) {
    trimmed.remove_suffix(1);
  }
  if (trimmed.empty()) {
    return false;
  }
  int base = 10;
  if (trimmed.front() == '#') {
    base = 16;
    trimmed.remove_prefix(1);
  } else if (trimmed.size() > 2 && trimmed.front() == '0' &&
             (trimmed[1] == 'x' || trimmed[1] == 'X')) {
    base = 16;
    trimmed.remove_prefix(2);
  } else {
    bool hasAlpha = false;
    for (char c : trimmed) {
      if (!std::isxdigit(static_cast<unsigned char>(c))) {
        hasAlpha = true;
        break;
      }
    }
    if (!hasAlpha) {
      base = 16;
    }
  }
  std::uint32_t value = 0;
  auto result = std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), value, base);
  if (result.ec != std::errc() || result.ptr != trimmed.data() + trimmed.size()) {
    return false;
  }
  color = colorFromInt(value);
  return true;
}

Ring::Color Ring::colorFromInt(std::uint32_t value) {
  Color c{};
  c.r = static_cast<std::uint8_t>((value >> 16) & 0xFFu);
  c.g = static_cast<std::uint8_t>((value >> 8) & 0xFFu);
  c.b = static_cast<std::uint8_t>(value & 0xFFu);
  return c;
}

void Ring::drawLine(avs::core::RenderContext& context, int x0, int y0, int x1, int y1,
                    const Color& color) {
  const int dx = std::abs(x1 - x0);
  const int sx = x0 < x1 ? 1 : -1;
  const int dy = -std::abs(y1 - y0);
  const int sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  while (true) {
    if (x0 >= 0 && y0 >= 0 && x0 < context.width && y0 < context.height) {
      const std::size_t index =
          (static_cast<std::size_t>(y0) * static_cast<std::size_t>(context.width) +
           static_cast<std::size_t>(x0)) *
          4u;
      if (index + 3 < context.framebuffer.size) {
        std::uint8_t* pixel = context.framebuffer.data + index;
        pixel[0] = color.r;
        pixel[1] = color.g;
        pixel[2] = color.b;
        pixel[3] = 255u;
      }
    }
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

int Ring::mirroredSegment(int segment) {
  const int half = kSegments / 2;
  return segment > half ? kSegments - segment : segment;
}

float Ring::samplePosition(int segment) {
  const int mirrored = mirroredSegment(segment);
  const float denom = static_cast<float>(kSegments / 2);
  if (denom <= 0.0f) {
    return 0.0f;
  }
  return std::clamp(static_cast<float>(mirrored) / denom, 0.0f, 1.0f);
}

void Ring::parseEffectBits(int effectBits) {
  channel_ = parseChannelFromInt((effectBits >> 2) & 3, channel_);
  const int placementBits = (effectBits >> 4) & 3;
  placement_ = parsePlacementFromInt(placementBits, placement_);
}

void Ring::parseChannelParam(const avs::core::ParamBlock& params) {
  if (params.contains("channel")) {
    const std::string channelStr = toLower(params.getString("channel", ""));
    if (!channelStr.empty()) {
      if (channelStr == "left") {
        channel_ = Channel::Left;
      } else if (channelStr == "right") {
        channel_ = Channel::Right;
      } else if (channelStr == "mid" || channelStr == "mix" || channelStr == "center") {
        channel_ = Channel::Mix;
      }
    } else {
      channel_ =
          parseChannelFromInt(params.getInt("channel", static_cast<int>(channel_)), channel_);
    }
  }
  if (params.contains("which_ch")) {
    channel_ = parseChannelFromInt(params.getInt("which_ch", 0), channel_);
  }
}

void Ring::parsePlacementParam(const avs::core::ParamBlock& params) {
  if (params.contains("placement")) {
    const std::string placementStr = toLower(params.getString("placement", ""));
    if (!placementStr.empty()) {
      if (placementStr == "left" || placementStr == "top") {
        placement_ = Placement::Left;
      } else if (placementStr == "right" || placementStr == "bottom") {
        placement_ = Placement::Right;
      } else if (placementStr == "center" || placementStr == "centre") {
        placement_ = Placement::Center;
      }
    } else {
      placement_ = parsePlacementFromInt(params.getInt("placement", static_cast<int>(placement_)),
                                         placement_);
    }
  }
  if (params.contains("position")) {
    placement_ = parsePlacementFromInt(params.getInt("position", 0), placement_);
  }
}

void Ring::parseSourceParam(const avs::core::ParamBlock& params) {
  if (params.contains("source")) {
    const std::string sourceStr = toLower(params.getString("source", ""));
    if (!sourceStr.empty()) {
      if (sourceStr == "osc" || sourceStr == "oscilloscope" || sourceStr == "wave") {
        source_ = Source::Oscilloscope;
      } else if (sourceStr == "spec" || sourceStr == "spectrum") {
        source_ = Source::Spectrum;
      }
    } else {
      source_ = parseSourceFromInt(params.getInt("source", static_cast<int>(source_)), source_);
    }
  }
}

void Ring::parseColorParams(const avs::core::ParamBlock& params) {
  bool updated = false;
  std::vector<Color> parsed;

  if (params.contains("colors")) {
    const std::string colorList = params.getString("colors", "");
    std::string_view view(colorList);
    while (!view.empty()) {
      const std::size_t pos = view.find_first_of(",;\n");
      const std::string_view token = view.substr(0, pos);
      Color color;
      if (parseColorToken(token, color)) {
        parsed.push_back(color);
      }
      if (pos == std::string_view::npos) {
        break;
      }
      view.remove_prefix(pos + 1);
    }
    updated = !parsed.empty();
  }

  if (!updated) {
    if (params.contains("color")) {
      parsed.push_back(colorFromInt(static_cast<std::uint32_t>(params.getInt("color", 0))));
      updated = true;
    }
    int count = 0;
    if (params.contains("num_colors")) {
      count = std::clamp(params.getInt("num_colors", 0), 0, 16);
    }
    if (count > 0) {
      for (int i = 0; i < count; ++i) {
        const std::string key = "color" + std::to_string(i);
        if (params.contains(key)) {
          parsed.push_back(colorFromInt(static_cast<std::uint32_t>(params.getInt(key, 0))));
        }
      }
      updated = !parsed.empty();
    } else {
      for (int i = 0; i < 16; ++i) {
        const std::string key = "color" + std::to_string(i);
        if (params.contains(key)) {
          parsed.push_back(colorFromInt(static_cast<std::uint32_t>(params.getInt(key, 0))));
        }
      }
      updated = !parsed.empty();
    }
  }

  if (updated) {
    colors_ = std::move(parsed);
    normalizeColorCursor();
  }
  if (colors_.empty()) {
    colors_.push_back(Color{255, 255, 255});
  }
}

Ring::Range Ring::waveformRange(std::size_t total) const {
  if (total == 0) {
    return {};
  }
  switch (channel_) {
    case Channel::Left: {
      const std::size_t count = std::max<std::size_t>(1, total / 2);
      return {0, count};
    }
    case Channel::Right: {
      const std::size_t half = total / 2;
      const std::size_t count = std::max<std::size_t>(1, total - half);
      return {half, count};
    }
    case Channel::Mix:
    default:
      return {0, total};
  }
}

Ring::Range Ring::spectrumRange(std::size_t total) const {
  if (total == 0) {
    return {};
  }
  switch (channel_) {
    case Channel::Left: {
      const std::size_t count = std::max<std::size_t>(1, total / 2);
      return {0, count};
    }
    case Channel::Right: {
      const std::size_t half = total / 2;
      const std::size_t count = std::max<std::size_t>(1, total - half);
      return {half, count};
    }
    case Channel::Mix:
    default:
      return {0, total};
  }
}

float Ring::sampleWaveform(const avs::audio::Analysis& analysis, Range range, int segment) const {
  if (range.count == 0) {
    return 0.5f;
  }
  const float position = samplePosition(segment);
  if (range.count == 1) {
    const float sample = analysis.waveform[range.begin];
    return std::clamp((sample + 1.0f) * 0.5f, 0.0f, 1.0f);
  }
  const std::size_t last = range.count - 1;
  const float scaled = position * static_cast<float>(last);
  const std::size_t index = range.begin + static_cast<std::size_t>(std::lround(scaled));
  const float sample = analysis.waveform[std::min<std::size_t>(range.begin + last, index)];
  return std::clamp((sample + 1.0f) * 0.5f, 0.0f, 1.0f);
}

float Ring::sampleSpectrum(const avs::audio::Analysis& analysis, Range range, float rangeMax,
                           int segment) const {
  if (range.count == 0 || rangeMax <= std::numeric_limits<float>::epsilon()) {
    return 0.0f;
  }
  const float position = samplePosition(segment);
  const std::size_t last = range.count > 0 ? range.count - 1 : 0;
  const float scaled = position * static_cast<float>(last);
  const std::size_t index = range.begin + static_cast<std::size_t>(std::lround(scaled));
  const std::size_t clampedIndex = std::min<std::size_t>(range.begin + last, index);
  const float sample = analysis.spectrum[clampedIndex];
  const float normalized = std::clamp(sample / rangeMax, 0.0f, 1.0f);
  return std::sqrt(normalized);
}

float Ring::sampleNormalized(const avs::audio::Analysis& analysis, Range waveform, Range spectrum,
                             float spectrumMax, int segment) const {
  if (source_ == Source::Oscilloscope) {
    return sampleWaveform(analysis, waveform, segment);
  }
  return sampleSpectrum(analysis, spectrum, spectrumMax, segment);
}

Ring::Color Ring::currentColor() const {
  if (colors_.empty()) {
    return Color{255, 255, 255};
  }
  const int cycle = static_cast<int>(colors_.size()) * kColorCycle;
  if (cycle <= 0) {
    return colors_.front();
  }
  const int pos = colorCursor_ % cycle;
  const int index = pos / kColorCycle;
  const int nextIndex = (index + 1) % static_cast<int>(colors_.size());
  const int remainder = pos % kColorCycle;
  const int weightCurrent = (kColorCycle - 1) - remainder;
  const int weightNext = remainder;
  const Color& a = colors_[static_cast<std::size_t>(index)];
  const Color& b = colors_[static_cast<std::size_t>(nextIndex)];
  Color blended{};
  blended.r = static_cast<std::uint8_t>(
      (static_cast<int>(a.r) * weightCurrent + static_cast<int>(b.r) * weightNext) / kColorCycle);
  blended.g = static_cast<std::uint8_t>(
      (static_cast<int>(a.g) * weightCurrent + static_cast<int>(b.g) * weightNext) / kColorCycle);
  blended.b = static_cast<std::uint8_t>(
      (static_cast<int>(a.b) * weightCurrent + static_cast<int>(b.b) * weightNext) / kColorCycle);
  return blended;
}

void Ring::normalizeColorCursor() {
  const int cycle = static_cast<int>(colors_.size()) * kColorCycle;
  if (cycle <= 0) {
    colorCursor_ = 0;
    return;
  }
  colorCursor_ %= cycle;
  if (colorCursor_ < 0) {
    colorCursor_ += cycle;
  }
}

void Ring::setParams(const avs::core::ParamBlock& params) {
  if (params.contains("size")) {
    size_ = clampSize(params.getInt("size", size_));
  }
  if (params.contains("effect")) {
    parseEffectBits(params.getInt("effect", 0));
  }
  parseChannelParam(params);
  parsePlacementParam(params);
  parseSourceParam(params);
  parseColorParams(params);
}

bool Ring::render(avs::core::RenderContext& context) {
  if (!context.framebuffer.data || context.width <= 0 || context.height <= 0) {
    return true;
  }
  const std::size_t required =
      static_cast<std::size_t>(context.width) * static_cast<std::size_t>(context.height) * 4u;
  if (context.framebuffer.size < required) {
    return false;
  }

  if (colors_.empty()) {
    colors_.push_back(Color{255, 255, 255});
  }
  const int cycle = static_cast<int>(colors_.size()) * kColorCycle;
  if (cycle > 0) {
    colorCursor_ = (colorCursor_ + 1) % cycle;
  }
  const Color color = currentColor();

  const float radiusScale = static_cast<float>(size_) / 32.0f;
  const float baseRadius =
      radiusScale * static_cast<float>(std::min(context.width, context.height));
  const float centerY = static_cast<float>(context.height) * 0.5f;
  float centerX = static_cast<float>(context.width) * 0.5f;
  switch (placement_) {
    case Placement::Left:
      centerX = static_cast<float>(context.width) * 0.25f;
      break;
    case Placement::Right:
      centerX = static_cast<float>(context.width) * 0.75f;
      break;
    case Placement::Center:
    default:
      centerX = static_cast<float>(context.width) * 0.5f;
      break;
  }

  const avs::audio::Analysis* analysis = context.audioAnalysis;
  Range waveform{};
  Range spectrum{};
  float spectrumMax = 0.0f;
  if (analysis) {
    waveform = waveformRange(analysis->waveform.size());
    spectrum = spectrumRange(analysis->spectrum.size());
    if (spectrum.count > 0) {
      for (std::size_t i = 0; i < spectrum.count; ++i) {
        spectrumMax = std::max(spectrumMax, analysis->spectrum[spectrum.begin + i]);
      }
    }
  }

  const auto sampleValue = [&](int segment) {
    if (!analysis) {
      return 0.5f;
    }
    return sampleNormalized(*analysis, waveform, spectrum, spectrumMax, segment);
  };

  float angle = 0.0f;
  float initialScale = 0.1f + sampleValue(0) * 0.9f;
  float prevX = centerX + std::cos(angle) * baseRadius * initialScale;
  float prevY = centerY + std::sin(angle) * baseRadius * initialScale;
  int prevXi = static_cast<int>(std::lround(prevX));
  int prevYi = static_cast<int>(std::lround(prevY));

  for (int segment = 1; segment <= kSegments; ++segment) {
    angle -= kTwoPi / static_cast<float>(kSegments);
    const float scale = 0.1f + sampleValue(segment) * 0.9f;
    const float x = centerX + std::cos(angle) * baseRadius * scale;
    const float y = centerY + std::sin(angle) * baseRadius * scale;
    const int xi = static_cast<int>(std::lround(x));
    const int yi = static_cast<int>(std::lround(y));
    drawLine(context, prevXi, prevYi, xi, yi, color);
    prevXi = xi;
    prevYi = yi;
  }

  return true;
}

}  // namespace avs::effects::render
