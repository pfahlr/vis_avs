#include "effects/render/effect_simple_spectrum.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

#include "audio/analyzer.h"
#include "avs/runtime/GlobalState.hpp"

namespace avs::effects::render {

namespace {

std::uint8_t clampByte(int value) {
  return static_cast<std::uint8_t>(std::clamp(value, 0, 255));
}

std::uint8_t saturatingAdd(std::uint8_t a, std::uint8_t b) {
  return static_cast<std::uint8_t>(std::min<int>(a + b, 255));
}

std::uint8_t channelMax(std::uint8_t a, std::uint8_t b) { return std::max(a, b); }

std::uint8_t channelMin(std::uint8_t a, std::uint8_t b) { return std::min(a, b); }

std::uint8_t subtractClamp(std::uint8_t a, std::uint8_t b) {
  return static_cast<std::uint8_t>(a > b ? a - b : 0u);
}

std::uint8_t averageChannel(std::uint8_t a, std::uint8_t b) {
  return static_cast<std::uint8_t>((static_cast<int>(a) + static_cast<int>(b) + 1) / 2);
}

std::uint8_t multiplyChannel(std::uint8_t a, std::uint8_t b) {
  return static_cast<std::uint8_t>((static_cast<int>(a) * static_cast<int>(b) + 127) / 255);
}

std::uint8_t blendAdjustChannel(std::uint8_t dst, std::uint8_t src, std::uint8_t alpha) {
  const std::uint8_t inv = static_cast<std::uint8_t>(255u - alpha);
  return static_cast<std::uint8_t>((static_cast<int>(dst) * inv + static_cast<int>(src) * alpha + 127) /
                                   255);
}

bool inBounds(const avs::core::RenderContext& context, int x, int y) {
  return x >= 0 && y >= 0 && x < context.width && y < context.height;
}

}  // namespace

SimpleSpectrum::SimpleSpectrum() {
  effectBits_ = (2 << 2) | (2 << 4);
  colors_.push_back(Color{255, 255, 255});
  spectrumState_.fill(0.0f);
}

void SimpleSpectrum::setParams(const avs::core::ParamBlock& params) {
  parseEffectBits(params);
  parseColors(params);
  normalizeColorCursor();
}

bool SimpleSpectrum::render(avs::core::RenderContext& context) {
  if (!context.framebuffer.data || context.width <= 0 || context.height <= 0) {
    return true;
  }

  if (colors_.empty()) {
    return true;
  }

  Rgba color{};
  const Color current = currentColor();
  color.r = current.r;
  color.g = current.g;
  color.b = current.b;
  color.a = 255u;

  const float yscale = static_cast<float>(context.height) / 2.0f / 256.0f;
  if (yscale == 0.0f) {
    return true;
  }

  if ((effectBits_ & (1 << 6)) != 0) {
    if (dotMode() == DotMode::Analyzer) {
      updateSpectrumState(context);
      renderDotAnalyzer(context, color, yscale);
    } else {
      renderDotScope(context, color, yscale);
    }
    return true;
  }

  switch (mode()) {
    case Mode::SolidAnalyzer:
      updateSpectrumState(context);
      renderSolidAnalyzer(context, color, yscale);
      break;
    case Mode::LineAnalyzer:
      updateSpectrumState(context);
      renderLineAnalyzer(context, color, yscale);
      break;
    case Mode::LineScope:
      renderLineScope(context, color, yscale);
      break;
    case Mode::SolidScope:
      renderSolidScope(context, color, yscale);
      break;
  }
  return true;
}

std::string SimpleSpectrum::toLower(std::string_view value) {
  std::string lowered(value);
  std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return lowered;
}

bool SimpleSpectrum::parseColorToken(std::string_view token, Color& color) {
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
    base = 16;
    token.remove_prefix(1);
  } else if (token.size() > 2 && token.front() == '0' &&
             (token[1] == 'x' || token[1] == 'X')) {
    base = 16;
    token.remove_prefix(2);
  } else {
    bool allHex = true;
    for (char c : token) {
      if (!std::isxdigit(static_cast<unsigned char>(c))) {
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

SimpleSpectrum::Color SimpleSpectrum::colorFromInt(std::uint32_t value) {
  Color color{};
  color.r = static_cast<std::uint8_t>((value >> 16) & 0xFFu);
  color.g = static_cast<std::uint8_t>((value >> 8) & 0xFFu);
  color.b = static_cast<std::uint8_t>(value & 0xFFu);
  return color;
}

void SimpleSpectrum::blendPixel(avs::core::RenderContext& context, int x, int y, const Rgba& color) {
  if (!context.framebuffer.data || !inBounds(context, x, y)) {
    return;
  }
  auto* pixel = context.framebuffer.data + (static_cast<std::size_t>(y) * context.width + x) * 4u;
  const avs::runtime::LegacyRenderState* legacy =
      (context.globals && context.globals->legacyRender.lineBlendModeActive)
          ? &context.globals->legacyRender
          : nullptr;

  if (!legacy) {
    pixel[0] = color.r;
    pixel[1] = color.g;
    pixel[2] = color.b;
    pixel[3] = color.a;
    return;
  }

  const std::uint8_t mode = static_cast<std::uint8_t>(legacy->lineBlendMode & 0xFFu);
  std::array<std::uint8_t, 4> source{color.r, color.g, color.b, color.a};
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
    case 7: {  // Adjustable blend
      const std::uint8_t alpha = static_cast<std::uint8_t>((legacy->lineBlendMode >> 8) & 0xFFu);
      pixel[0] = blendAdjustChannel(pixel[0], source[0], alpha);
      pixel[1] = blendAdjustChannel(pixel[1], source[1], alpha);
      pixel[2] = blendAdjustChannel(pixel[2], source[2], alpha);
      pixel[3] = blendAdjustChannel(pixel[3], source[3], alpha);
      break;
    }
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
      pixel[0] = color.r;
      pixel[1] = color.g;
      pixel[2] = color.b;
      pixel[3] = color.a;
      break;
  }
}

void SimpleSpectrum::drawLine(avs::core::RenderContext& context,
                              int x0,
                              int y0,
                              int x1,
                              int y1,
                              const Rgba& color) {
  int dx = std::abs(x1 - x0);
  int sx = x0 < x1 ? 1 : -1;
  int dy = -std::abs(y1 - y0);
  int sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  while (true) {
    blendPixel(context, x0, y0, color);
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

void SimpleSpectrum::drawVerticalLine(avs::core::RenderContext& context,
                                      int x,
                                      int y0,
                                      int y1,
                                      const Rgba& color) {
  if (y0 > y1) {
    std::swap(y0, y1);
  }
  for (int y = y0; y <= y1; ++y) {
    blendPixel(context, x, y, color);
  }
}

void SimpleSpectrum::parseEffectBits(const avs::core::ParamBlock& params) {
  if (params.contains("effect")) {
    effectBits_ = params.getInt("effect", effectBits_);
  }

  if (params.contains("which_ch")) {
    const int channelBits = params.getInt("which_ch", 0) & 3;
    effectBits_ &= ~(3 << 2);
    effectBits_ |= (channelBits << 2);
  }

  if (params.contains("y_pos")) {
    const int posBits = params.getInt("y_pos", 0) & 3;
    effectBits_ &= ~(3 << 4);
    effectBits_ |= (posBits << 4);
  }

  if (params.contains("dot")) {
    if (params.getBool("dot", false)) {
      effectBits_ |= (1 << 6);
    } else {
      effectBits_ &= ~(1 << 6);
    }
  }
}

void SimpleSpectrum::parseColors(const avs::core::ParamBlock& params) {
  std::vector<Color> parsed;
  const std::string colorList = params.getString("colors", "");
  if (!colorList.empty()) {
    std::string token;
    for (char c : colorList) {
      if (c == ',' || c == ';' || std::isspace(static_cast<unsigned char>(c))) {
        if (!token.empty()) {
          Color color{};
          if (parseColorToken(token, color)) {
            parsed.push_back(color);
          }
          token.clear();
        }
      } else {
        token.push_back(c);
      }
    }
    if (!token.empty()) {
      Color color{};
      if (parseColorToken(token, color)) {
        parsed.push_back(color);
      }
    }
  }

  const int requested = std::clamp(params.getInt("num_colors", 0), 0, kMaxColors);
  for (int i = 0; i < requested; ++i) {
    const std::string key = "color" + std::to_string(i);
    if (params.contains(key)) {
      parsed.push_back(colorFromInt(static_cast<std::uint32_t>(params.getInt(key, 0))));
    }
  }

  if (parsed.empty()) {
    for (int i = 0; i < kMaxColors; ++i) {
      const std::string key = "color" + std::to_string(i);
      if (params.contains(key)) {
        parsed.push_back(colorFromInt(static_cast<std::uint32_t>(params.getInt(key, 0))));
      }
    }
  }

  if (parsed.empty() && params.contains("color")) {
    parsed.push_back(colorFromInt(static_cast<std::uint32_t>(params.getInt("color", 0))));
  }

  if (!parsed.empty()) {
    if (parsed.size() > static_cast<std::size_t>(kMaxColors)) {
      parsed.resize(static_cast<std::size_t>(kMaxColors));
    }
    colors_ = std::move(parsed);
  }
}

void SimpleSpectrum::normalizeColorCursor() {
  if (colors_.empty()) {
    colorCursor_ = 0;
    return;
  }
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

SimpleSpectrum::Color SimpleSpectrum::currentColor() {
  if (colors_.empty()) {
    return Color{255, 255, 255};
  }
  if (colors_.size() == 1u) {
    colorCursor_ = (colorCursor_ + 1) % kColorCycle;
    return colors_.front();
  }
  const int totalSteps = static_cast<int>(colors_.size()) * kColorCycle;
  if (totalSteps <= 0) {
    return colors_.front();
  }
  colorCursor_ = (colorCursor_ + 1) % totalSteps;
  const int index = colorCursor_ / kColorCycle;
  const int frac = colorCursor_ % kColorCycle;
  const Color& a = colors_[static_cast<std::size_t>(index)];
  const Color& b = colors_[static_cast<std::size_t>((index + 1) % colors_.size())];
  Color result{};
  result.r = clampByte((static_cast<int>(a.r) * (kColorCycle - frac) + static_cast<int>(b.r) * frac) /
                       kColorCycle);
  result.g = clampByte((static_cast<int>(a.g) * (kColorCycle - frac) + static_cast<int>(b.g) * frac) /
                       kColorCycle);
  result.b = clampByte((static_cast<int>(a.b) * (kColorCycle - frac) + static_cast<int>(b.b) * frac) /
                       kColorCycle);
  return result;
}

SimpleSpectrum::Mode SimpleSpectrum::mode() const {
  return static_cast<Mode>(effectBits_ & 3);
}

SimpleSpectrum::DotMode SimpleSpectrum::dotMode() const {
  return (effectBits_ & 2) != 0 ? DotMode::Scope : DotMode::Analyzer;
}

int SimpleSpectrum::placement() const { return (effectBits_ >> 4) & 3; }

void SimpleSpectrum::updateSpectrumState(const avs::core::RenderContext& context) {
  const float* spectrum = nullptr;
  std::size_t size = 0;
  if (context.audioSpectrum.data && context.audioSpectrum.size > 0) {
    spectrum = context.audioSpectrum.data;
    size = context.audioSpectrum.size;
  } else if (context.audioAnalysis) {
    spectrum = context.audioAnalysis->spectrum.data();
    size = context.audioAnalysis->spectrum.size();
  }

  if (!spectrum || size == 0) {
    decaySpectrumState();
    return;
  }

  std::array<float, kAnalyzerBands> raw{};
  const double bandWidth = static_cast<double>(size) / static_cast<double>(kAnalyzerBands);
  float maxRaw = 0.0f;
  for (int band = 0; band < kAnalyzerBands; ++band) {
    const double start = static_cast<double>(band) * bandWidth;
    double end = start + bandWidth;
    std::size_t beginIndex = static_cast<std::size_t>(std::floor(start));
    std::size_t endIndex = static_cast<std::size_t>(std::floor(end));
    if (endIndex <= beginIndex) {
      endIndex = std::min(beginIndex + 1, size);
    }
    float sum = 0.0f;
    std::size_t count = 0;
    for (std::size_t i = beginIndex; i < endIndex && i < size; ++i) {
      const float value = spectrum[i];
      if (std::isfinite(value) && value > 0.0f) {
        sum += value;
      }
      ++count;
    }
    float value = 0.0f;
    if (count > 0) {
      value = sum / static_cast<float>(count);
    }
    raw[static_cast<std::size_t>(band)] = value;
    maxRaw = std::max(maxRaw, value);
  }

  if (maxRaw > 0.0f) {
    normalization_ = std::max(maxRaw, normalization_ * kNormalizationDecay);
  } else {
    normalization_ *= kNormalizationDecay;
  }
  if (normalization_ < 1e-3f) {
    normalization_ = 1e-3f;
  }

  const float scale = 255.0f / normalization_;
  for (int band = 0; band < kAnalyzerBands; ++band) {
    float target = raw[static_cast<std::size_t>(band)] * scale;
    target = std::clamp(target, 0.0f, 255.0f);
    float current = spectrumState_[static_cast<std::size_t>(band)];
    if (target >= current) {
      current = target;
    } else {
      current *= kSpectrumDecay;
      if (current < target) {
        current = target;
      }
    }
    spectrumState_[static_cast<std::size_t>(band)] = std::clamp(current, 0.0f, 255.0f);
  }
}

void SimpleSpectrum::decaySpectrumState() {
  normalization_ *= kNormalizationDecay;
  if (normalization_ < 1e-3f) {
    normalization_ = 1e-3f;
  }
  for (float& value : spectrumState_) {
    value *= kSpectrumDecay;
    if (value < 0.01f) {
      value = 0.0f;
    }
  }
}

float SimpleSpectrum::sampleSpectrum(float index) const {
  if (spectrumState_.empty()) {
    return 0.0f;
  }
  if (index <= 0.0f) {
    return spectrumState_.front();
  }
  const float maxIndex = static_cast<float>(kAnalyzerBands - 1);
  if (index >= maxIndex) {
    return spectrumState_.back();
  }
  const int base = static_cast<int>(index);
  const int next = std::min(base + 1, kAnalyzerBands - 1);
  const float frac = index - static_cast<float>(base);
  const float a = spectrumState_[static_cast<std::size_t>(base)];
  const float b = spectrumState_[static_cast<std::size_t>(next)];
  return a * (1.0f - frac) + b * frac;
}

void SimpleSpectrum::sampleWaveform(const avs::core::RenderContext& context,
                                    std::array<float, kWaveformSamples>& samples) const {
  samples.fill(0.0f);
  const avs::audio::Analysis* analysis = context.audioAnalysis;
  if (!analysis || analysis->waveform.empty()) {
    return;
  }
  const std::size_t sourceSize = analysis->waveform.size();
  const double scale = static_cast<double>(sourceSize) / static_cast<double>(kWaveformSamples);
  for (int i = 0; i < kWaveformSamples; ++i) {
    const double start = static_cast<double>(i) * scale;
    double end = start + scale;
    std::size_t beginIndex = static_cast<std::size_t>(std::floor(start));
    std::size_t endIndex = static_cast<std::size_t>(std::floor(end));
    if (endIndex <= beginIndex) {
      endIndex = std::min(beginIndex + 1, sourceSize);
    }
    float sum = 0.0f;
    std::size_t count = 0;
    for (std::size_t j = beginIndex; j < endIndex && j < sourceSize; ++j) {
      float sample = analysis->waveform[j];
      if (!std::isfinite(sample)) {
        sample = 0.0f;
      }
      sum += sample;
      ++count;
    }
    float value = 0.0f;
    if (count > 0) {
      value = sum / static_cast<float>(count);
    }
    value = std::clamp(value, -1.0f, 1.0f) * 127.0f;
    samples[static_cast<std::size_t>(i)] = value;
  }
}

float SimpleSpectrum::sampleWaveformAt(const std::array<float, kWaveformSamples>& samples,
                                       float index) const {
  if (index <= 0.0f) {
    return samples.front();
  }
  const float maxIndex = static_cast<float>(kWaveformSamples - 1);
  if (index >= maxIndex) {
    return samples.back();
  }
  const int base = static_cast<int>(index);
  const int next = std::min(base + 1, kWaveformSamples - 1);
  const float frac = index - static_cast<float>(base);
  const float a = samples[static_cast<std::size_t>(base)];
  const float b = samples[static_cast<std::size_t>(next)];
  return a * (1.0f - frac) + b * frac;
}

void SimpleSpectrum::renderDotAnalyzer(avs::core::RenderContext& context,
                                       const Rgba& color,
                                       float yscale) {
  if (context.width <= 0) {
    return;
  }
  const float xscale = static_cast<float>(kAnalyzerBands) / static_cast<float>(context.width);
  int h2 = context.height / 2;
  float ys = yscale;
  int adj = 1;
  const int pos = placement();
  if (pos != 1) {
    ys = -ys;
    adj = 0;
  }
  if (pos == 2) {
    h2 -= static_cast<int>(ys * 256.0f / 2.0f);
  }

  for (int x = 0; x < context.width; ++x) {
    const float position = static_cast<float>(x) * xscale;
    const float value = sampleSpectrum(position);
    const int y = h2 + adj + static_cast<int>(value * ys - 1.0f);
    if (y >= 0 && y < context.height) {
      blendPixel(context, x, y, color);
    }
  }
}

void SimpleSpectrum::renderDotScope(avs::core::RenderContext& context,
                                    const Rgba& color,
                                    float yscale) {
  if (context.width <= 0) {
    return;
  }
  std::array<float, kWaveformSamples> waveform{};
  sampleWaveform(context, waveform);
  const float xscale = static_cast<float>(kWaveformSamples) / static_cast<float>(context.width);
  int yh = 0;
  const int pos = placement();
  if (pos == 2) {
    yh = context.height / 4;
  } else {
    yh = pos * context.height / 2;
  }

  for (int x = 0; x < context.width; ++x) {
    const float position = static_cast<float>(x) * xscale;
    const float value = sampleWaveformAt(waveform, position);
    const int y = yh + static_cast<int>(value * yscale);
    if (y >= 0 && y < context.height) {
      blendPixel(context, x, y, color);
    }
  }
}

void SimpleSpectrum::renderSolidAnalyzer(avs::core::RenderContext& context,
                                         const Rgba& color,
                                         float yscale) {
  if (context.width <= 0) {
    return;
  }
  const float xscale = static_cast<float>(kAnalyzerBands) / static_cast<float>(context.width);
  int h2 = context.height / 2;
  float ys = yscale;
  int adj = 1;
  const int pos = placement();
  if (pos != 1) {
    ys = -ys;
    adj = 0;
  }
  if (pos == 2) {
    h2 -= static_cast<int>(ys * 256.0f / 2.0f);
  }

  for (int x = 0; x < context.width; ++x) {
    const float position = static_cast<float>(x) * xscale;
    const float value = sampleSpectrum(position);
    const int y = h2 + adj + static_cast<int>(value * ys - 1.0f);
    drawVerticalLine(context, x, h2 - adj, y, color);
  }
}

void SimpleSpectrum::renderLineAnalyzer(avs::core::RenderContext& context,
                                        const Rgba& color,
                                        float yscale) {
  if (context.width <= 0) {
    return;
  }
  const float xs = static_cast<float>(context.width) / static_cast<float>(kAnalyzerBands);
  float ys = yscale;
  int h2 = context.height / 2;
  const int pos = placement();
  if (pos != 1) {
    ys = -ys;
  }
  if (pos == 2) {
    h2 -= static_cast<int>(ys * 256.0f / 2.0f);
  }

  int lx = 0;
  int ly = h2 + static_cast<int>(sampleSpectrum(0.0f) * ys);
  for (int band = 1; band < kAnalyzerBands; ++band) {
    const int ox = static_cast<int>(static_cast<float>(band) * xs);
    const int oy = h2 + static_cast<int>(sampleSpectrum(static_cast<float>(band)) * ys);
    drawLine(context, lx, ly, ox, oy, color);
    lx = ox;
    ly = oy;
  }
}

void SimpleSpectrum::renderLineScope(avs::core::RenderContext& context,
                                     const Rgba& color,
                                     float yscale) {
  if (context.width <= 0) {
    return;
  }
  std::array<float, kWaveformSamples> waveform{};
  sampleWaveform(context, waveform);
  const float xs = static_cast<float>(context.width) / static_cast<float>(kWaveformSamples);
  int yh = 0;
  const int pos = placement();
  if (pos == 2) {
    yh = context.height / 4;
  } else {
    yh = pos * context.height / 2;
  }

  int lx = 0;
  int ly = yh + static_cast<int>(sampleWaveformAt(waveform, 0.0f) * yscale);
  for (int i = 1; i < kWaveformSamples; ++i) {
    const int ox = static_cast<int>(static_cast<float>(i) * xs);
    const int oy = yh + static_cast<int>(sampleWaveformAt(waveform, static_cast<float>(i)) * yscale);
    drawLine(context, lx, ly, ox, oy, color);
    lx = ox;
    ly = oy;
  }
}

void SimpleSpectrum::renderSolidScope(avs::core::RenderContext& context,
                                      const Rgba& color,
                                      float yscale) {
  if (context.width <= 0) {
    return;
  }
  std::array<float, kWaveformSamples> waveform{};
  sampleWaveform(context, waveform);
  const float xscale = static_cast<float>(kWaveformSamples) / static_cast<float>(context.width);
  int yh = 0;
  const int pos = placement();
  if (pos == 2) {
    yh = context.height / 4;
  } else {
    yh = pos * context.height / 2;
  }
  const int baseline = yh + static_cast<int>(yscale * 128.0f);

  for (int x = 0; x < context.width; ++x) {
    const float position = static_cast<float>(x) * xscale;
    const float value = sampleWaveformAt(waveform, position);
    const int y = yh + static_cast<int>(value * yscale);
    drawVerticalLine(context, x, baseline - 1, y, color);
  }
}

}  // namespace avs::effects::render

