#include "avs/effects/AudioOverlays.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "audio/analyzer.h"
#include "avs/core/RenderContext.hpp"

namespace avs::effects {
namespace {

using Color = std::array<std::uint8_t, 4>;

Color makeColor(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255) {
  return Color{r, g, b, a};
}

int clampInt(int value, int minValue, int maxValue) {
  return std::min(std::max(value, minValue), maxValue);
}

struct Glyph {
  std::array<std::uint8_t, 5> columns{};
};

const std::map<char, Glyph>& glyphMap() {
  static const std::map<char, Glyph> glyphs = {
      {'0', {{0x3E, 0x51, 0x49, 0x45, 0x3E}}}, {'1', {{0x00, 0x42, 0x7F, 0x40, 0x00}}},
      {'2', {{0x42, 0x61, 0x51, 0x49, 0x46}}}, {'3', {{0x21, 0x41, 0x45, 0x4B, 0x31}}},
      {'4', {{0x18, 0x14, 0x12, 0x7F, 0x10}}}, {'5', {{0x27, 0x45, 0x45, 0x45, 0x39}}},
      {'6', {{0x3C, 0x4A, 0x49, 0x49, 0x30}}}, {'7', {{0x01, 0x71, 0x09, 0x05, 0x03}}},
      {'8', {{0x36, 0x49, 0x49, 0x49, 0x36}}}, {'9', {{0x06, 0x49, 0x49, 0x29, 0x1E}}},
      {'A', {{0x7E, 0x11, 0x11, 0x11, 0x7E}}}, {'B', {{0x7F, 0x49, 0x49, 0x49, 0x36}}},
      {'C', {{0x3E, 0x41, 0x41, 0x41, 0x22}}}, {'D', {{0x7F, 0x41, 0x41, 0x22, 0x1C}}},
      {'E', {{0x7F, 0x49, 0x49, 0x49, 0x41}}}, {'F', {{0x7F, 0x09, 0x09, 0x09, 0x01}}},
      {'G', {{0x3E, 0x41, 0x49, 0x49, 0x7A}}}, {'H', {{0x7F, 0x08, 0x08, 0x08, 0x7F}}},
      {'I', {{0x00, 0x41, 0x7F, 0x41, 0x00}}}, {'J', {{0x20, 0x40, 0x41, 0x3F, 0x01}}},
      {'K', {{0x7F, 0x08, 0x14, 0x22, 0x41}}}, {'L', {{0x7F, 0x40, 0x40, 0x40, 0x40}}},
      {'M', {{0x7F, 0x02, 0x04, 0x02, 0x7F}}}, {'N', {{0x7F, 0x04, 0x08, 0x10, 0x7F}}},
      {'O', {{0x3E, 0x41, 0x41, 0x41, 0x3E}}}, {'P', {{0x7F, 0x09, 0x09, 0x09, 0x06}}},
      {'Q', {{0x3E, 0x41, 0x51, 0x21, 0x5E}}}, {'R', {{0x7F, 0x09, 0x19, 0x29, 0x46}}},
      {'S', {{0x46, 0x49, 0x49, 0x49, 0x31}}}, {'T', {{0x01, 0x01, 0x7F, 0x01, 0x01}}},
      {'U', {{0x3F, 0x40, 0x40, 0x40, 0x3F}}}, {'V', {{0x1F, 0x20, 0x40, 0x20, 0x1F}}},
      {'W', {{0x7F, 0x20, 0x18, 0x20, 0x7F}}}, {'X', {{0x63, 0x14, 0x08, 0x14, 0x63}}},
      {'Y', {{0x07, 0x08, 0x70, 0x08, 0x07}}}, {'Z', {{0x61, 0x51, 0x49, 0x45, 0x43}}},
      {':', {{0x00, 0x36, 0x36, 0x00, 0x00}}}, {' ', {{0x00, 0x00, 0x00, 0x00, 0x00}}},
      {'.', {{0x00, 0x40, 0x60, 0x00, 0x00}}}, {'%', {{0x62, 0x64, 0x08, 0x13, 0x23}}},
  };
  return glyphs;
}

const Glyph* glyphFor(char c) {
  const auto& glyphs = glyphMap();
  auto it = glyphs.find(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
  if (it != glyphs.end()) {
    return &it->second;
  }
  return nullptr;
}

std::string toUpperCopy(const std::string& text) {
  std::string result = text;
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
  return result;
}

}  // namespace

AudioOverlay::AudioOverlay(Mode mode) : mode_(mode) {}

bool AudioOverlay::render(avs::core::RenderContext& context) {
  if (!context.framebuffer.data || context.framebuffer.size == 0 || !context.audioAnalysis) {
    return true;
  }

  if (mode_ == Mode::Wave) {
    Color color = waveColor_;
    if (highlightBeat_ && context.audioAnalysis->beat) {
      color = makeColor(255, 80, 80, 255);
    }
    drawWave(context, color);
  } else if (mode_ == Mode::Spectrum) {
    drawSpectrum(context);
  } else if (mode_ == Mode::Bands) {
    drawBands(context);
  } else if (mode_ == Mode::LevelText) {
    drawLevelText(context);
  } else if (mode_ == Mode::BandText) {
    drawBandText(context);
  }
  return true;
}

void AudioOverlay::setParams(const avs::core::ParamBlock& params) {
  waveColor_ = parseColor(params, "color", waveColor_);
  textColor_ = parseColor(params, "text_color", textColor_);
  gain_ = std::max(0.01f, params.getFloat("gain", gain_));
  highlightBeat_ = params.getBool("beat", highlightBeat_);
  if (params.contains("damp")) {
    const int numeric = params.getInt("damp", damping_ ? 1 : 0);
    damping_ = numeric != 0 || params.getBool("damp", damping_);
  }
}

AudioOverlay::Color AudioOverlay::parseColor(const avs::core::ParamBlock& params,
                                             const std::string& key, Color fallback) {
  if (!params.contains(key)) {
    return fallback;
  }
  std::string value = params.getString(key, "");
  if (!value.empty()) {
    std::string hex = value;
    if (hex.front() == '#') {
      hex.erase(hex.begin());
    }
    if (hex.size() == 6) {
      auto parseComponent = [&](std::string_view slice) -> std::uint8_t {
        return static_cast<std::uint8_t>(std::stoi(std::string(slice), nullptr, 16));
      };
      try {
        const std::uint8_t r = parseComponent({hex.data(), 2});
        const std::uint8_t g = parseComponent({hex.data() + 2, 2});
        const std::uint8_t b = parseComponent({hex.data() + 4, 2});
        return makeColor(r, g, b, fallback[3]);
      } catch (...) {
        return fallback;
      }
    }
  }
  const int numeric = params.getInt(key, -1);
  if (numeric >= 0) {
    const std::uint8_t r = static_cast<std::uint8_t>((numeric >> 16) & 0xFF);
    const std::uint8_t g = static_cast<std::uint8_t>((numeric >> 8) & 0xFF);
    const std::uint8_t b = static_cast<std::uint8_t>(numeric & 0xFF);
    return makeColor(r, g, b, fallback[3]);
  }
  return fallback;
}

void AudioOverlay::putPixel(avs::core::RenderContext& context, int x, int y, const Color& color) {
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

void AudioOverlay::fillRect(avs::core::RenderContext& context, int x0, int y0, int x1, int y1,
                            const Color& color) {
  x0 = clampInt(x0, 0, context.width);
  y0 = clampInt(y0, 0, context.height);
  x1 = clampInt(x1, 0, context.width);
  y1 = clampInt(y1, 0, context.height);
  if (x0 >= x1 || y0 >= y1) {
    return;
  }
  const int stride = context.width * 4;
  std::uint8_t* base = context.framebuffer.data + y0 * stride + x0 * 4;
  for (int y = y0; y < y1; ++y) {
    std::uint8_t* row = base + (y - y0) * stride;
    for (int x = x0; x < x1; ++x) {
      std::uint8_t* px = row + (x - x0) * 4;
      px[0] = color[0];
      px[1] = color[1];
      px[2] = color[2];
      px[3] = color[3];
    }
  }
}

void AudioOverlay::drawLine(avs::core::RenderContext& context, int x0, int y0, int x1, int y1,
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
    const int e2 = 2 * err;
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

void AudioOverlay::drawPolyline(avs::core::RenderContext& context,
                                const std::vector<float>& samples, float scaleY,
                                const Color& color) const {
  if (samples.empty()) {
    return;
  }
  const int midY = context.height / 2;
  const int width = context.width;
  int prevX = 0;
  int prevY = midY;
  for (int x = 0; x < width; ++x) {
    const float position = static_cast<float>(x) / std::max(1, width - 1);
    const std::size_t index = static_cast<std::size_t>(position * (samples.size() - 1));
    const float sample = std::clamp(samples[index] * gain_, -1.0f, 1.0f);
    const int y = midY - static_cast<int>(sample * scaleY);
    if (x > 0) {
      drawLine(context, prevX, prevY, x, y, color);
    }
    prevX = x;
    prevY = y;
  }
}

AudioOverlay::Color AudioOverlay::gradient(float t) {
  const std::array<Color, 5> stops = {makeColor(25, 25, 112), makeColor(30, 144, 255),
                                      makeColor(60, 179, 113), makeColor(255, 215, 0),
                                      makeColor(220, 20, 60)};
  t = std::clamp(t, 0.0f, 1.0f);
  const float scaled = t * static_cast<float>(stops.size() - 1);
  const int index = static_cast<int>(std::floor(scaled));
  const float frac = scaled - static_cast<float>(index);
  const Color& a = stops[static_cast<std::size_t>(index)];
  const Color& b = stops[static_cast<std::size_t>(std::min<int>(index + 1, stops.size() - 1))];
  Color result{};
  for (int i = 0; i < 4; ++i) {
    result[static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(std::lround(
        a[static_cast<std::size_t>(i)] * (1.0f - frac) + b[static_cast<std::size_t>(i)] * frac));
  }
  return result;
}

void AudioOverlay::drawWave(avs::core::RenderContext& context, const Color& color) const {
  const auto& analysis = *context.audioAnalysis;
  if (waveCache_.size() != analysis.waveform.size()) {
    waveCache_.assign(analysis.waveform.size(), 0.0f);
  }
  for (std::size_t i = 0; i < analysis.waveform.size(); ++i) {
    const float target = analysis.waveform[i];
    if (damping_) {
      waveCache_[i] = waveCache_[i] * 0.75f + target * 0.25f;
    } else {
      waveCache_[i] = target;
    }
  }
  drawPolyline(context, waveCache_, static_cast<float>(context.height) * 0.45f, color);
}

void AudioOverlay::drawSpectrum(avs::core::RenderContext& context) const {
  const auto& analysis = *context.audioAnalysis;
  const int width = context.width;
  const int height = context.height;
  const std::size_t spectrumSize = analysis.spectrum.size();
  if (spectrumSize == 0) {
    return;
  }
  if (spectrumCache_.size() != spectrumSize) {
    spectrumCache_.assign(spectrumSize, 0.0f);
  }
  const int barWidth = std::max(1, width / static_cast<int>(spectrumSize));
  for (int x = 0; x < width; ++x) {
    const std::size_t index =
        std::min<std::size_t>(spectrumSize - 1, static_cast<std::size_t>(x * spectrumSize / width));
    const float magnitude = analysis.spectrum[index];
    if (damping_) {
      spectrumCache_[index] = spectrumCache_[index] * 0.7f + magnitude * 0.3f;
    } else {
      spectrumCache_[index] = magnitude;
    }
    const float smoothed = spectrumCache_[index];
    const float normalized = std::clamp(smoothed * gain_, 0.0f, 1.0f);
    const int barHeight = static_cast<int>(normalized * static_cast<float>(height));
    const Color color = gradient(normalized);
    fillRect(context, x, height - barHeight, x + barWidth, height, color);
  }
}

void AudioOverlay::drawBands(avs::core::RenderContext& context) const {
  const auto& analysis = *context.audioAnalysis;
  const std::array<float, 3> values = {analysis.bass, analysis.mid, analysis.treb};
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (damping_) {
      bandCache_[i] = bandCache_[i] * 0.6f + values[i] * 0.4f;
    } else {
      bandCache_[i] = values[i];
    }
  }
  const int width = context.width;
  const int height = context.height;
  const int bandWidth = std::max(1, width / 3);
  for (int i = 0; i < 3; ++i) {
    const float normalized =
        std::clamp(bandCache_[static_cast<std::size_t>(i)] * gain_, 0.0f, 1.0f);
    const Color color = gradient(normalized);
    const int x0 = i * bandWidth;
    const int x1 = (i == 2) ? width : x0 + bandWidth;
    const int barHeight = static_cast<int>(normalized * static_cast<float>(height));
    fillRect(context, x0, height - barHeight, x1, height, color);
    if (highlightBeat_ && context.audioAnalysis->beat && i == 0) {
      fillRect(context, x0, 0, x1, std::min(4, height), makeColor(255, 255, 255));
    }
  }
}

std::string AudioOverlay::formatFloat(float value, int precision) {
  char buffer[32];
  std::snprintf(buffer, sizeof(buffer), "%.*f", precision, value);
  return std::string(buffer);
}

void AudioOverlay::drawText(avs::core::RenderContext& context, int originX, int originY,
                            const std::string& text, const Color& color) const {
  int x = originX;
  for (char ch : text) {
    if (ch == '\n') {
      originY += 8;
      x = originX;
      continue;
    }
    const Glyph* glyph = glyphFor(ch);
    if (!glyph) {
      x += 6;
      continue;
    }
    for (std::size_t col = 0; col < glyph->columns.size(); ++col) {
      const std::uint8_t column = glyph->columns[col];
      for (int row = 0; row < 7; ++row) {
        if (column & (1u << row)) {
          putPixel(context, x + static_cast<int>(col), originY + row, color);
        }
      }
    }
    x += static_cast<int>(glyph->columns.size()) + 1;
  }
}

void AudioOverlay::drawLevelText(avs::core::RenderContext& context) const {
  const auto& analysis = *context.audioAnalysis;
  const float rms = std::min(1.0f, analysis.confidence * 2.0f);
  std::string line = "LEVEL " + formatFloat(rms * 100.0f, 1) + "%";
  if (analysis.beat) {
    line += " BEAT";
  }
  if (analysis.bpm > 1.0f) {
    line += " BPM " + formatFloat(analysis.bpm, 1);
  }
  drawText(context, 4, 4, toUpperCopy(line), textColor_);
}

void AudioOverlay::drawBandText(avs::core::RenderContext& context) const {
  const auto& analysis = *context.audioAnalysis;
  std::string text = "BASS " + formatFloat(analysis.bass * gain_, 2) + " MID " +
                     formatFloat(analysis.mid * gain_, 2) + " TREB " +
                     formatFloat(analysis.treb * gain_, 2);
  drawText(context, 4, context.height - 12, toUpperCopy(text), textColor_);
}

}  // namespace avs::effects
