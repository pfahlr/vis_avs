#include <avs/effects/legacy/render/effect_timescope.h>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <limits>

#include <avs/audio/analyzer.h>

namespace avs::effects::render {
namespace {
constexpr int kMinBands = 16;
constexpr int kMaxBands = 576;
constexpr float kSpectrumDecay = 0.88f;
constexpr float kNormalizationDecay = 0.96f;
constexpr float kMinNormalization = 1e-3f;
}  // namespace

Timescope::Timescope() { color_ = colorFromInt(0xFFFFFFu); }

bool Timescope::hasFramebuffer(const avs::core::RenderContext& context) {
  if (!context.framebuffer.data || context.width <= 0 || context.height <= 0) {
    return false;
  }
  const std::size_t required =
      static_cast<std::size_t>(context.width) * static_cast<std::size_t>(context.height) * 4u;
  return context.framebuffer.size >= required;
}

std::uint8_t Timescope::saturatingAdd(std::uint8_t base, std::uint8_t add) {
  const int sum = static_cast<int>(base) + static_cast<int>(add);
  return static_cast<std::uint8_t>(std::min(sum, 255));
}

Timescope::Color Timescope::colorFromInt(std::uint32_t value) {
  Color color{};
  color.r = static_cast<std::uint8_t>(value & 0xFFu);
  color.g = static_cast<std::uint8_t>((value >> 8u) & 0xFFu);
  color.b = static_cast<std::uint8_t>((value >> 16u) & 0xFFu);
  return color;
}

bool Timescope::parseColorToken(std::string_view token, std::uint32_t& value) {
  if (token.empty()) {
    return false;
  }
  int base = 10;
  if (token.front() == '#') {
    token.remove_prefix(1);
    base = 16;
  } else if (token.size() > 2 && token[0] == '0' && (token[1] == 'x' || token[1] == 'X')) {
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
  if (token.empty()) {
    return false;
  }
  std::uint32_t parsed = 0;
  const auto result = std::from_chars(token.data(), token.data() + token.size(), parsed, base);
  if (result.ec != std::errc() || result.ptr != token.data() + token.size()) {
    return false;
  }
  value = parsed;
  return true;
}

std::uint32_t Timescope::parseColorString(const std::string& value, std::uint32_t fallback) {
  std::string token;
  token.reserve(value.size());
  for (char ch : value) {
    if (std::isspace(static_cast<unsigned char>(ch))) {
      if (!token.empty()) {
        break;
      }
      continue;
    }
    if (ch == ',' || ch == ';') {
      if (!token.empty()) {
        break;
      }
      continue;
    }
    token.push_back(ch);
  }
  if (token.empty()) {
    return fallback;
  }
  std::uint32_t parsed = 0;
  if (parseColorToken(token, parsed)) {
    return parsed;
  }
  return fallback;
}

void Timescope::ensureBandCapacity() {
  const std::size_t desired =
      static_cast<std::size_t>(std::clamp(bandCount_, kMinBands, kMaxBands));
  if (bandState_.size() != desired) {
    bandState_.assign(desired, 0.0f);
  }
  if (scratchBands_.size() != desired) {
    scratchBands_.assign(desired, 0.0f);
  }
}

void Timescope::applyParams(const avs::core::ParamBlock& params) {
  if (params.contains("enabled")) {
    enabled_ = params.getInt("enabled", enabled_ ? 1 : 0) != 0;
  }

  if (params.contains("blend")) {
    const int blend = params.getInt("blend", 2);
    if (blend <= 0) {
      blendMode_ = BlendMode::Replace;
    } else if (blend == 1) {
      blendMode_ = BlendMode::Additive;
    } else {
      blendMode_ = BlendMode::Line;
    }
  }

  blendAverage_ = params.getInt("blendavg", blendAverage_ ? 1 : 0) != 0;

  const std::uint32_t fallback = static_cast<std::uint32_t>(color_.r) |
                                 (static_cast<std::uint32_t>(color_.g) << 8u) |
                                 (static_cast<std::uint32_t>(color_.b) << 16u);

  if (params.contains("color")) {
    color_ = colorFromInt(static_cast<std::uint32_t>(params.getInt("color", fallback)));
  }
  if (params.contains("colour")) {
    const auto parsed = parseColorString(params.getString("colour", ""), fallback);
    color_ = colorFromInt(parsed);
  }
  if (params.contains("colors")) {
    const auto parsed = parseColorString(params.getString("colors", ""), fallback);
    color_ = colorFromInt(parsed);
  }

  bandCount_ = std::clamp(params.getInt("nbands", bandCount_), kMinBands, kMaxBands);
  if (params.contains("which_ch")) {
    channelSelection_ = params.getInt("which_ch", channelSelection_);
    channelSelection_ = std::clamp(channelSelection_, 0, 2);
  }

  ensureBandCapacity();
}

void Timescope::decaySpectrumState() {
  normalization_ *= kNormalizationDecay;
  if (normalization_ < kMinNormalization) {
    normalization_ = kMinNormalization;
  }
  for (float& value : bandState_) {
    value *= kSpectrumDecay;
    if (value < 0.01f) {
      value = 0.0f;
    }
  }
}

void Timescope::updateSpectrumState(const avs::core::RenderContext& context) {
  const float* spectrum = nullptr;
  std::size_t size = 0;
  if (context.audioSpectrum.data && context.audioSpectrum.size > 0) {
    spectrum = context.audioSpectrum.data;
    size = context.audioSpectrum.size;
  } else if (context.audioAnalysis) {
    spectrum = context.audioAnalysis->spectrum.data();
    size = context.audioAnalysis->spectrum.size();
  }

  if (!spectrum || size == 0 || bandState_.empty()) {
    decaySpectrumState();
    return;
  }

  const std::size_t bands = bandState_.size();
  const double bandWidth = static_cast<double>(size) / static_cast<double>(bands);

  float maxValue = 0.0f;
  for (std::size_t band = 0; band < bands; ++band) {
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
    const float average = count > 0 ? sum / static_cast<float>(count) : 0.0f;
    scratchBands_[band] = average;
    maxValue = std::max(maxValue, average);
  }

  if (maxValue > 0.0f) {
    normalization_ = std::max(maxValue, normalization_ * kNormalizationDecay);
  } else {
    normalization_ *= kNormalizationDecay;
  }
  if (normalization_ < kMinNormalization) {
    normalization_ = kMinNormalization;
  }

  const float scale = 255.0f / normalization_;
  for (std::size_t i = 0; i < bands; ++i) {
    float target = scratchBands_[i] * scale;
    if (!std::isfinite(target)) {
      target = 0.0f;
    }
    target = std::clamp(target, 0.0f, 255.0f);
    float current = bandState_[i];
    if (target >= current) {
      current = target;
    } else {
      current *= kSpectrumDecay;
      if (current < target) {
        current = target;
      }
    }
    if (current < 0.01f) {
      current = 0.0f;
    }
    bandState_[i] = current;
  }
}

Timescope::Color Timescope::scaleColor(float intensity) const {
  const int scaled = static_cast<int>(std::clamp(intensity, 0.0f, 255.0f));
  Color result{};
  result.r = static_cast<std::uint8_t>((static_cast<int>(color_.r) * scaled) / 255);
  result.g = static_cast<std::uint8_t>((static_cast<int>(color_.g) * scaled) / 255);
  result.b = static_cast<std::uint8_t>((static_cast<int>(color_.b) * scaled) / 255);
  return result;
}

void Timescope::applyBlend(std::uint8_t* pixel, const Color& color) const {
  if (blendMode_ == BlendMode::Replace) {
    if (blendAverage_) {
      pixel[0] = static_cast<std::uint8_t>((static_cast<int>(pixel[0]) + color.r) / 2);
      pixel[1] = static_cast<std::uint8_t>((static_cast<int>(pixel[1]) + color.g) / 2);
      pixel[2] = static_cast<std::uint8_t>((static_cast<int>(pixel[2]) + color.b) / 2);
    } else {
      pixel[0] = color.r;
      pixel[1] = color.g;
      pixel[2] = color.b;
    }
  } else {
    pixel[0] = saturatingAdd(pixel[0], color.r);
    pixel[1] = saturatingAdd(pixel[1], color.g);
    pixel[2] = saturatingAdd(pixel[2], color.b);
  }
  pixel[3] = 255u;
}

void Timescope::setParams(const avs::core::ParamBlock& params) { applyParams(params); }

bool Timescope::render(avs::core::RenderContext& context) {
  if (!enabled_) {
    return true;
  }
  if (!hasFramebuffer(context)) {
    return true;
  }

  ensureBandCapacity();
  updateSpectrumState(context);

  if (bandState_.empty()) {
    return true;
  }

  const int width = context.width;
  const int height = context.height;
  cursor_ += 1;
  if (cursor_ >= width || cursor_ < 0) {
    cursor_ = 0;
  }

  std::uint8_t* framebuffer = context.framebuffer.data;
  const std::size_t bands = bandState_.size();
  for (int y = 0; y < height; ++y) {
    std::size_t bandIndex = 0;
    if (height > 0) {
      const std::size_t scaledY = static_cast<std::size_t>(y) * bands;
      bandIndex = scaledY / static_cast<std::size_t>(height);
      if (bandIndex >= bands) {
        bandIndex = bands - 1;
      }
    }
    const float intensity = bandState_[bandIndex];
    const Color sample = scaleColor(intensity);
    const std::size_t index = (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                               static_cast<std::size_t>(cursor_)) *
                              4u;
    applyBlend(framebuffer + index, sample);
  }

  return true;
}

}  // namespace avs::effects::render
