#include "effects/render/effect_simple_spectrum.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <string>

#include "avs/core/RenderContext.hpp"
#include "audio/analyzer.h"

namespace {

constexpr int kMaxColors = 16;
constexpr int kColorPhase = 64;
constexpr double kLogBase = 60.0;
constexpr double kLogBaseInv = 1.0 / std::log(kLogBase);
constexpr double kSpectrumScale = 255.0;
constexpr double kWaveScale = 127.0;

std::uint8_t clampToByte(double value) {
  if (value < 0.0) {
    return 0;
  }
  if (value > 255.0) {
    return 255;
  }
  return static_cast<std::uint8_t>(std::lround(value));
}

}  // namespace

namespace avs::effects::render {

SimpleSpectrum::SimpleSpectrum() {
  colors_.push_back(Color{255, 255, 255, 255});
}

bool SimpleSpectrum::render(avs::core::RenderContext& context) {
  if (context.width <= 0 || context.height <= 0) {
    return true;
  }
  if (!context.framebuffer.data ||
      context.framebuffer.size < static_cast<std::size_t>(context.width) *
                                     static_cast<std::size_t>(context.height) * 4u) {
    return true;
  }
  if (colors_.empty()) {
    return true;
  }

  prepareAudioData(context);
  advanceColorCycle();
  const Color color = currentColor();

  const bool dotMode = (effectBits_ & (1 << 6)) != 0;
  const int mode = effectBits_ & 3;
  const int whichChannel = (effectBits_ >> 2) & 3;
  const int yPos = (effectBits_ >> 4) & 3;

  const bool useScope = mode > 1 || (dotMode && (effectBits_ & 2));
  const std::array<float, 576>& left = useScope ? scopeLeft_ : analyzerLeft_;
  const std::array<float, 576>& right = useScope ? scopeRight_ : analyzerRight_;
  const std::array<float, 576>& center = useScope ? scopeCenter_ : analyzerCenter_;

  const float* data = nullptr;
  switch (whichChannel) {
    case 0:
      data = left.data();
      break;
    case 1:
      data = right.data();
      break;
    case 2:
    case 3:
      data = center.data();
      break;
    default:
      data = left.data();
      break;
  }
  if (!data) {
    return true;
  }

  std::uint8_t* framebuffer = context.framebuffer.data;
  const std::size_t bufferSize = context.framebuffer.size;
  const int width = context.width;
  const int height = context.height;

  const float yScale = static_cast<float>(height) / 2.0f / 256.0f;
  const float xScale = 288.0f / static_cast<float>(std::max(1, width));

  auto sampleLinear = [&](float position, int limit) {
    if (position <= 0.0f) {
      return data[0];
    }
    const float maxIndex = static_cast<float>(limit);
    if (position >= maxIndex) {
      return data[limit];
    }
    const int base = static_cast<int>(position);
    const float frac = position - static_cast<float>(base);
    const float a = data[base];
    const float b = data[std::min(base + 1, limit)];
    return a * (1.0f - frac) + b * frac;
  };

  if (dotMode) {
    if (effectBits_ & 2) {
      const int yBase = (yPos == 2) ? height / 4 : (yPos * height) / 2;
      for (int x = 0; x < width; ++x) {
        const float sample = sampleLinear(static_cast<float>(x) * xScale, 287);
        const int y = yBase + static_cast<int>(sample * yScale);
        putPixel(framebuffer, bufferSize, width, height, x, y, color);
      }
    } else {
      int h2 = height / 2;
      float ys = yScale;
      int adj = 1;
      if (yPos != 1) {
        ys = -ys;
        adj = 0;
      }
      if (yPos == 2) {
        h2 -= static_cast<int>(ys * 256.0f / 2.0f);
      }
      const float xs = 200.0f / static_cast<float>(std::max(1, width));
      for (int x = 0; x < width; ++x) {
        const float sample = sampleLinear(static_cast<float>(x) * xs, 199);
        const int y = h2 + adj + static_cast<int>(sample * ys - 1.0f);
        putPixel(framebuffer, bufferSize, width, height, x, y, color);
      }
    }
    return true;
  }

  switch (mode) {
    case 0: {
      int h2 = height / 2;
      float ys = yScale;
      int adj = 1;
      if (yPos != 1) {
        ys = -ys;
        adj = 0;
      }
      if (yPos == 2) {
        h2 -= static_cast<int>(ys * 256.0f / 2.0f);
      }
      const float xs = 200.0f / static_cast<float>(std::max(1, width));
      for (int x = 0; x < width; ++x) {
        const float sample = sampleLinear(static_cast<float>(x) * xs, 199);
        const int y0 = h2 - adj;
        const int y1 = h2 + adj + static_cast<int>(sample * ys - 1.0f);
        drawVerticalLine(framebuffer, bufferSize, width, height, x, y0, y1, color);
      }
      break;
    }
    case 1: {
      int h2 = height / 2;
      float ys = yScale;
      if (yPos != 1) {
        ys = -ys;
      }
      if (yPos == 2) {
        h2 -= static_cast<int>(ys * 256.0f / 2.0f);
      }
      const float xs = static_cast<float>(width) / 200.0f;
      int lastX = 0;
      int lastY = h2 + static_cast<int>(data[0] * ys);
      for (int i = 1; i < 200; ++i) {
        const int nextX = static_cast<int>(std::lround(static_cast<float>(i) * xs));
        const int nextY = h2 + static_cast<int>(data[i] * ys);
        drawLine(framebuffer, bufferSize, width, height, lastX, lastY, nextX, nextY, color);
        lastX = nextX;
        lastY = nextY;
      }
      break;
    }
    case 2: {
      const float xs = static_cast<float>(width) / 288.0f;
      const int baseY = (yPos == 2) ? height / 4 : (yPos * height) / 2;
      int lastX = 0;
      int lastY = baseY + static_cast<int>(data[0] * yScale);
      for (int i = 1; i < 288; ++i) {
        const int nextX = static_cast<int>(std::lround(static_cast<float>(i) * xs));
        const int nextY = baseY + static_cast<int>(data[i] * yScale);
        drawLine(framebuffer, bufferSize, width, height, lastX, lastY, nextX, nextY, color);
        lastX = nextX;
        lastY = nextY;
      }
      break;
    }
    case 3: {
      int yh = (yPos == 2) ? height / 4 : (yPos * height) / 2;
      const int ys = yh + static_cast<int>(yScale * 128.0f);
      for (int x = 0; x < width; ++x) {
        const float sample = sampleLinear(static_cast<float>(x) * xScale, 287);
        const int y = yh + static_cast<int>(sample * yScale);
        drawVerticalLine(framebuffer, bufferSize, width, height, x, ys - 1, y, color);
      }
      break;
    }
    default:
      break;
  }

  return true;
}

void SimpleSpectrum::setParams(const avs::core::ParamBlock& params) {
  updateEffectBits(params);
  updateColors(params);
}

void SimpleSpectrum::updateEffectBits(const avs::core::ParamBlock& params) {
  if (params.contains("effect")) {
    effectBits_ = params.getInt("effect", effectBits_);
  }
  if (params.contains("mode")) {
    const int mode = params.getInt("mode", effectBits_ & 3) & 3;
    effectBits_ = (effectBits_ & ~3) | mode;
  }
  if (params.contains("channel")) {
    const int ch = params.getInt("channel", (effectBits_ >> 2) & 3) & 3;
    effectBits_ = (effectBits_ & ~(3 << 2)) | (ch << 2);
  }
  if (params.contains("position")) {
    const int pos = params.getInt("position", (effectBits_ >> 4) & 3) & 3;
    effectBits_ = (effectBits_ & ~(3 << 4)) | (pos << 4);
  }
  if (params.contains("dot_mode")) {
    const bool dot = params.getBool("dot_mode", (effectBits_ & (1 << 6)) != 0);
    if (dot) {
      effectBits_ |= (1 << 6);
    } else {
      effectBits_ &= ~(1 << 6);
    }
  }
  if (params.contains("dot")) {
    const bool dot = params.getBool("dot", (effectBits_ & (1 << 6)) != 0);
    if (dot) {
      effectBits_ |= (1 << 6);
    } else {
      effectBits_ &= ~(1 << 6);
    }
  }
}

void SimpleSpectrum::updateColors(const avs::core::ParamBlock& params) {
  std::vector<Color> parsed;
  int declared = -1;
  if (params.contains("num_colors")) {
    declared = params.getInt("num_colors", -1);
  } else if (params.contains("color_count")) {
    declared = params.getInt("color_count", -1);
  }
  auto readColor = [&](const std::string& key) {
    if (!params.contains(key)) {
      return false;
    }
    const std::uint32_t value = static_cast<std::uint32_t>(params.getInt(key, 0x00FFFFFF));
    Color color{};
    color.r = static_cast<std::uint8_t>((value >> 16) & 0xFFu);
    color.g = static_cast<std::uint8_t>((value >> 8) & 0xFFu);
    color.b = static_cast<std::uint8_t>(value & 0xFFu);
    color.a = 255;
    parsed.push_back(color);
    return true;
  };

  if (declared > 0) {
    const int limit = std::min(declared, kMaxColors);
    for (int i = 0; i < limit; ++i) {
      const std::string key = "color" + std::to_string(i);
      if (!readColor(key)) {
        break;
      }
    }
  } else {
    for (int i = 0; i < kMaxColors; ++i) {
      const std::string key = "color" + std::to_string(i);
      if (!readColor(key)) {
        if (declared >= 0) {
          break;
        }
      }
    }
  }

  if (!parsed.empty()) {
    colors_ = std::move(parsed);
    if (colorPosition_ >= static_cast<int>(colors_.size()) * kColorPhase) {
      colorPosition_ = 0;
    }
  } else if (colors_.empty()) {
    colors_.push_back(Color{255, 255, 255, 255});
    colorPosition_ = 0;
  }
}

SimpleSpectrum::Color SimpleSpectrum::currentColor() const {
  if (colors_.empty()) {
    return Color{255, 255, 255, 255};
  }
  const int count = static_cast<int>(colors_.size());
  const int phase = std::max(1, count * kColorPhase);
  const int position = colorPosition_ % phase;
  const int index = position / kColorPhase;
  const int frac = position % kColorPhase;
  const Color& a = colors_[index];
  const Color& b = colors_[(index + 1) % count];
  const int weightA = kColorPhase - frac;
  const int weightB = frac;
  Color result{};
  result.r = clampToByte((static_cast<double>(a.r) * weightA + static_cast<double>(b.r) * weightB) /
                         static_cast<double>(kColorPhase));
  result.g = clampToByte((static_cast<double>(a.g) * weightA + static_cast<double>(b.g) * weightB) /
                         static_cast<double>(kColorPhase));
  result.b = clampToByte((static_cast<double>(a.b) * weightA + static_cast<double>(b.b) * weightB) /
                         static_cast<double>(kColorPhase));
  result.a = 255;
  return result;
}

void SimpleSpectrum::advanceColorCycle() {
  if (colors_.empty()) {
    return;
  }
  const int total = std::max(1, static_cast<int>(colors_.size()) * kColorPhase);
  colorPosition_ = (colorPosition_ + 1) % total;
}

void SimpleSpectrum::prepareAudioData(const avs::core::RenderContext& context) {
  auto clearArrays = [&]() {
    analyzerLeft_.fill(0.0f);
    analyzerRight_.fill(0.0f);
    analyzerCenter_.fill(0.0f);
    scopeLeft_.fill(0.0f);
    scopeRight_.fill(0.0f);
    scopeCenter_.fill(0.0f);
  };

  if (!context.audioAnalysis && (!context.audioSpectrum.data || context.audioSpectrum.size == 0)) {
    clearArrays();
    return;
  }

  std::vector<float> spectrum;
  std::vector<float> waveform;
  if (context.audioAnalysis) {
    const auto& analysis = *context.audioAnalysis;
    spectrum.assign(analysis.spectrum.begin(), analysis.spectrum.end());
    waveform.assign(analysis.waveform.begin(), analysis.waveform.end());
  }
  if (spectrum.empty() && context.audioSpectrum.data && context.audioSpectrum.size > 0) {
    spectrum.assign(context.audioSpectrum.data,
                    context.audioSpectrum.data + context.audioSpectrum.size);
  }

  if (spectrum.empty()) {
    analyzerLeft_.fill(0.0f);
    analyzerRight_.fill(0.0f);
    analyzerCenter_.fill(0.0f);
  } else {
    const double maxValue = std::max(1e-6, static_cast<double>(*std::max_element(spectrum.begin(), spectrum.end())));
    const double scale = kSpectrumScale / maxValue;
    const std::size_t limit = spectrum.size() > 1 ? spectrum.size() - 1 : 0;
    for (std::size_t i = 0; i < analyzerLeft_.size(); ++i) {
      const double t = analyzerLeft_.size() > 1 ? static_cast<double>(i) /
                                                   static_cast<double>(analyzerLeft_.size() - 1)
                                               : 0.0;
      const double pos = t * static_cast<double>(limit);
      const std::size_t base = static_cast<std::size_t>(std::floor(pos));
      const std::size_t next = std::min(base + 1, limit);
      const double frac = pos - static_cast<double>(base);
      const double a = spectrum[base];
      const double b = spectrum[next];
      const double sample = a * (1.0 - frac) + b * frac;
      const double raw = std::clamp(sample * scale, 0.0, 255.0);
      const double mapped = std::log(raw * (kLogBase / 255.0) + 1.0) * kLogBaseInv * 255.0;
      const float value = static_cast<float>(std::clamp(mapped, 0.0, 255.0));
      analyzerLeft_[i] = value;
      analyzerRight_[i] = value;
      analyzerCenter_[i] = value;
    }
  }

  if (waveform.empty()) {
    scopeLeft_.fill(0.0f);
    scopeRight_.fill(0.0f);
    scopeCenter_.fill(0.0f);
  } else {
    const std::size_t limit = waveform.size() > 1 ? waveform.size() - 1 : 0;
    for (std::size_t i = 0; i < scopeLeft_.size(); ++i) {
      const double t = scopeLeft_.size() > 1 ? static_cast<double>(i) /
                                                static_cast<double>(scopeLeft_.size() - 1)
                                            : 0.0;
      const double pos = t * static_cast<double>(limit);
      const std::size_t base = static_cast<std::size_t>(std::floor(pos));
      const std::size_t next = std::min(base + 1, limit);
      const double frac = pos - static_cast<double>(base);
      const double a = waveform[base];
      const double b = waveform[next];
      const double sample = a * (1.0 - frac) + b * frac;
      const double clamped = std::clamp(sample, -1.0, 1.0);
      const float value = static_cast<float>(clamped * kWaveScale);
      scopeLeft_[i] = value;
      scopeRight_[i] = value;
      scopeCenter_[i] = value;
    }
  }
}

void SimpleSpectrum::putPixel(std::uint8_t* framebuffer, std::size_t bufferSize, int width,
                              int height, int x, int y, const Color& color) {
  if (x < 0 || y < 0 || x >= width || y >= height) {
    return;
  }
  const std::size_t index = (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                             static_cast<std::size_t>(x)) *
                            4u;
  if (index + 3 >= bufferSize) {
    return;
  }
  framebuffer[index + 0] = color.r;
  framebuffer[index + 1] = color.g;
  framebuffer[index + 2] = color.b;
  framebuffer[index + 3] = color.a;
}

void SimpleSpectrum::drawLine(std::uint8_t* framebuffer, std::size_t bufferSize, int width,
                              int height, int x0, int y0, int x1, int y1, const Color& color) {
  int dx = std::abs(x1 - x0);
  int sx = x0 < x1 ? 1 : -1;
  int dy = -std::abs(y1 - y0);
  int sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;

  while (true) {
    putPixel(framebuffer, bufferSize, width, height, x0, y0, color);
    if (x0 == x1 && y0 == y1) {
      break;
    }
    const int e2 = err << 1;
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

void SimpleSpectrum::drawVerticalLine(std::uint8_t* framebuffer, std::size_t bufferSize,
                                      int width, int height, int x, int y0, int y1,
                                      const Color& color) {
  if (y0 > y1) {
    std::swap(y0, y1);
  }
  for (int y = y0; y <= y1; ++y) {
    putPixel(framebuffer, bufferSize, width, height, x, y, color);
  }
}

}  // namespace avs::effects::render

