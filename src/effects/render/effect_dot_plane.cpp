#include "effects/render/effect_dot_plane.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>
#include <string>
#include <string_view>

#include "audio/analyzer.h"
#include "avs/core/RenderContext.hpp"

namespace {

constexpr float kCellSpan = 350.0f;
constexpr float kDampingFactor = 0.15f;
constexpr float kHeightBias = 64.0f;
constexpr float kPerspectiveBaseWidth = 640.0f;
constexpr float kPerspectiveBaseHeight = 480.0f;
constexpr float kPerspectiveDepth = 440.0f;
constexpr float kMinProjectionZ = 1.0f;

struct Matrix4 {
  std::array<float, 16> m{};

  static Matrix4 identity() {
    Matrix4 mat{};
    mat.m = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
             0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    return mat;
  }

  static Matrix4 rotationX(float degrees) {
    const float radians = static_cast<float>(degrees) * std::numbers::pi_v<float> / 180.0f;
    const float c = std::cos(radians);
    const float s = std::sin(radians);
    Matrix4 mat = identity();
    mat.m[5] = c;
    mat.m[6] = s;
    mat.m[9] = -s;
    mat.m[10] = c;
    return mat;
  }

  static Matrix4 rotationY(float degrees) {
    const float radians = static_cast<float>(degrees) * std::numbers::pi_v<float> / 180.0f;
    const float c = std::cos(radians);
    const float s = std::sin(radians);
    Matrix4 mat = identity();
    mat.m[0] = c;
    mat.m[2] = -s;
    mat.m[8] = s;
    mat.m[10] = c;
    return mat;
  }

  static Matrix4 translation(float x, float y, float z) {
    Matrix4 mat = identity();
    mat.m[3] = x;
    mat.m[7] = y;
    mat.m[11] = z;
    return mat;
  }
};

Matrix4 multiply(const Matrix4& a, const Matrix4& b) {
  Matrix4 result{};
  for (int row = 0; row < 4; ++row) {
    for (int col = 0; col < 4; ++col) {
      float sum = 0.0f;
      for (int k = 0; k < 4; ++k) {
        sum += a.m[row * 4 + k] * b.m[k * 4 + col];
      }
      result.m[row * 4 + col] = sum;
    }
  }
  return result;
}

std::array<float, 3> transformPoint(const Matrix4& matrix, float x, float y, float z) {
  return {x * matrix.m[0] + y * matrix.m[1] + z * matrix.m[2] + matrix.m[3],
          x * matrix.m[4] + y * matrix.m[5] + z * matrix.m[6] + matrix.m[7],
          x * matrix.m[8] + y * matrix.m[9] + z * matrix.m[10] + matrix.m[11]};
}

}  // namespace

namespace avs::effects::render {

namespace {
std::array<Rgb, 5> defaultPalette() {
  return {{{Rgb{24, 107, 28}},
           {Rgb{35, 10, 255}},
           {Rgb{116, 29, 42}},
           {Rgb{217, 54, 144}},
           {Rgb{255, 136, 107}}}};
}

std::string colorKey(std::string_view prefix, std::size_t index) {
  std::string key(prefix);
  key += std::to_string(index);
  return key;
}

}  // namespace

DotPlane::DotPlane() : palette_(defaultPalette()) {}

std::uint8_t DotPlane::clampByte(int value) {
  return static_cast<std::uint8_t>(value <= 0 ? 0 : (value >= 255 ? 255 : value));
}

std::uint8_t DotPlane::saturatingAdd(std::uint8_t base, std::uint8_t add) {
  const int sum = static_cast<int>(base) + static_cast<int>(add);
  return static_cast<std::uint8_t>(sum > 255 ? 255 : sum);
}

Rgb DotPlane::decodeColor(int value) {
  const std::uint32_t v = static_cast<std::uint32_t>(value);
  return {static_cast<std::uint8_t>((v >> 16) & 0xFFu), static_cast<std::uint8_t>((v >> 8) & 0xFFu),
          static_cast<std::uint8_t>(v & 0xFFu)};
}

std::uint32_t DotPlane::encodeColor(const Rgb& color) {
  return static_cast<std::uint32_t>(color.r) | (static_cast<std::uint32_t>(color.g) << 8u) |
         (static_cast<std::uint32_t>(color.b) << 16u);
}

bool DotPlane::applyColorParam(const avs::core::ParamBlock& params, std::string_view prefix,
                               std::size_t index) {
  const std::string key = colorKey(prefix, index);
  if (!params.contains(key)) {
    return false;
  }
  const int value = params.getInt(key, 0);
  const Rgb decoded = decodeColor(value);
  if (decoded.r == palette_[index].r && decoded.g == palette_[index].g &&
      decoded.b == palette_[index].b) {
    return false;
  }
  palette_[index] = decoded;
  paletteDirty_ = true;
  return true;
}

void DotPlane::rebuildColorGradient() {
  for (std::size_t segment = 0; segment < palette_.size() - 1; ++segment) {
    const Rgb& from = palette_[segment];
    const Rgb& to = palette_[segment + 1];
    for (int step = 0; step < 16; ++step) {
      const float t = static_cast<float>(step) / 16.0f;
      const auto interpolateChannel = [t](std::uint8_t a, std::uint8_t b) {
        const float value =
            static_cast<float>(a) + (static_cast<float>(b) - static_cast<float>(a)) * t;
        return clampByte(static_cast<int>(std::round(value)));
      };
      const Rgb color{interpolateChannel(from.r, to.r), interpolateChannel(from.g, to.g),
                      interpolateChannel(from.b, to.b)};
      colorGradient_[segment * 16 + step] = encodeColor(color);
    }
  }
  paletteDirty_ = false;
}

void DotPlane::sampleAudio(const avs::core::RenderContext& context,
                           std::array<float, kGridSize>& amplitudes) const {
  amplitudes.fill(0.0f);
  if (context.audioAnalysis) {
    const auto& waveform = context.audioAnalysis->waveform;
    if (!waveform.empty()) {
      const std::size_t samples = waveform.size();
      const std::size_t slice = std::max<std::size_t>(1, samples / kGridSize);
      for (int column = 0; column < kGridSize; ++column) {
        const std::size_t start = static_cast<std::size_t>(column) * slice;
        const std::size_t end = std::min(samples, start + slice);
        float peak = 0.0f;
        for (std::size_t i = start; i < end; ++i) {
          peak = std::max(peak, std::abs(waveform[i]));
        }
        amplitudes[column] = std::clamp(peak * 255.0f, 0.0f, 255.0f);
      }
    }
  }

  if (context.audioSpectrum.data && context.audioSpectrum.size > 0) {
    const float* spectrum = context.audioSpectrum.data;
    const std::size_t bins = context.audioSpectrum.size;
    const std::size_t slice = std::max<std::size_t>(1, bins / kGridSize);
    for (int column = 0; column < kGridSize; ++column) {
      const std::size_t start = static_cast<std::size_t>(column) * slice;
      const std::size_t end = std::min(bins, start + slice);
      float sum = 0.0f;
      for (std::size_t i = start; i < end; ++i) {
        sum += spectrum[i];
      }
      const float average = (end > start) ? sum / static_cast<float>(end - start) : 0.0f;
      const float scaled = std::clamp(average * 32.0f, 0.0f, 255.0f);
      amplitudes[column] = std::max(amplitudes[column], scaled);
    }
  }
}

void DotPlane::updateHeightField(const std::array<float, kGridSize>& previousTop,
                                 const std::array<float, kGridSize>& newTop) {
  for (int row = kGridSize - 1; row >= 1; --row) {
    for (int column = 0; column < kGridSize; ++column) {
      const std::size_t dst =
          static_cast<std::size_t>(row) * kGridSize + static_cast<std::size_t>(column);
      const std::size_t src =
          static_cast<std::size_t>(row - 1) * kGridSize + static_cast<std::size_t>(column);
      float value = height_[src] + velocity_[src];
      if (value < 0.0f) {
        value = 0.0f;
      }
      height_[dst] = value;
      velocity_[dst] = velocity_[src] - kDampingFactor * (value / 255.0f);
      colorRows_[dst] = colorRows_[src];
    }
  }

  for (int column = 0; column < kGridSize; ++column) {
    const std::size_t index = static_cast<std::size_t>(column);
    const float value = std::clamp(newTop[column], 0.0f, 255.0f);
    height_[index] = value;
    velocity_[index] = (value - previousTop[column]) / 90.0f;
    const int gradientIndex = static_cast<int>(value / 4.0f);
    const int clampedIndex =
        std::clamp(gradientIndex, 0, static_cast<int>(colorGradient_.size() - 1));
    colorRows_[index] = colorGradient_[static_cast<std::size_t>(clampedIndex)];
  }
}

bool DotPlane::render(avs::core::RenderContext& context) {
  if (!context.framebuffer.data || context.width <= 0 || context.height <= 0) {
    return true;
  }
  const std::size_t required =
      static_cast<std::size_t>(context.width) * static_cast<std::size_t>(context.height) * 4u;
  if (context.framebuffer.size < required) {
    return false;
  }

  if (paletteDirty_) {
    rebuildColorGradient();
  }

  std::array<float, kGridSize> previousTop{};
  std::copy_n(height_.begin(), kGridSize, previousTop.begin());

  std::array<float, kGridSize> newTop{};
  sampleAudio(context, newTop);
  updateHeightField(previousTop, newTop);

  Matrix4 transform = Matrix4::translation(0.0f, -20.0f, 400.0f);
  transform = multiply(transform, Matrix4::rotationX(static_cast<float>(tiltDegrees_)));
  transform = multiply(transform, Matrix4::rotationY(rotationDegrees_));

  float perspective = static_cast<float>(context.width) * kPerspectiveDepth / kPerspectiveBaseWidth;
  const float alt = static_cast<float>(context.height) * kPerspectiveDepth / kPerspectiveBaseHeight;
  if (alt < perspective) {
    perspective = alt;
  }

  const float cellWidth = kCellSpan / static_cast<float>(kGridSize);
  std::uint8_t* pixels = context.framebuffer.data;

  for (int fo = 0; fo < kGridSize; ++fo) {
    const bool flip = rotationDegrees_ < 90.0f || rotationDegrees_ > 270.0f;
    const int baseRow = flip ? (kGridSize - fo - 1) : fo;
    const float baseZ =
        (static_cast<float>(baseRow) - static_cast<float>(kGridSize) * 0.5f) * cellWidth;

    const std::size_t rowOffset = static_cast<std::size_t>(baseRow) * kGridSize;
    const float initialW = -(static_cast<float>(kGridSize) * 0.5f) * cellWidth;

    float w = initialW;
    const float* heightPtr = height_.data() + rowOffset;
    const std::uint32_t* colorPtr = colorRows_.data() + rowOffset;
    int step = 1;
    if (rotationDegrees_ < 180.0f) {
      step = -1;
      w = -w - cellWidth;
      heightPtr += kGridSize - 1;
      colorPtr += kGridSize - 1;
    }

    for (int column = 0; column < kGridSize; ++column) {
      const float heightValue = kHeightBias - *heightPtr;
      const auto pos = transformPoint(transform, w, heightValue, baseZ);
      if (pos[2] > kMinProjectionZ) {
        const float invZ = perspective / pos[2];
        const int ix = static_cast<int>(pos[0] * invZ) + context.width / 2;
        const int iy = static_cast<int>(pos[1] * invZ) + context.height / 2;
        if (ix >= 0 && ix < context.width && iy >= 0 && iy < context.height) {
          std::uint8_t* pixel =
              pixels + (static_cast<std::size_t>(iy) * static_cast<std::size_t>(context.width) +
                        static_cast<std::size_t>(ix)) *
                           4u;
          const std::uint32_t color = *colorPtr;
          const std::uint8_t r = static_cast<std::uint8_t>(color & 0xFFu);
          const std::uint8_t g = static_cast<std::uint8_t>((color >> 8u) & 0xFFu);
          const std::uint8_t b = static_cast<std::uint8_t>((color >> 16u) & 0xFFu);
          pixel[0] = saturatingAdd(pixel[0], r);
          pixel[1] = saturatingAdd(pixel[1], g);
          pixel[2] = saturatingAdd(pixel[2], b);
        }
      }
      w += cellWidth * static_cast<float>(step);
      heightPtr += step;
      colorPtr += step;
    }
  }

  rotationDegrees_ += static_cast<float>(rotationVelocity_) / 5.0f;
  if (rotationDegrees_ >= 360.0f) {
    rotationDegrees_ -= 360.0f;
  }
  if (rotationDegrees_ < 0.0f) {
    rotationDegrees_ += 360.0f;
  }

  return true;
}

void DotPlane::setParams(const avs::core::ParamBlock& params) {
  const int rotationVelocity =
      params.getInt("rotvel", params.getInt("rotation_velocity", rotationVelocity_));
  rotationVelocity_ = std::clamp(rotationVelocity, -128, 128);

  int tilt = tiltDegrees_;
  if (params.contains("angle")) {
    tilt = params.getInt("angle", tilt);
  } else if (params.contains("tilt")) {
    tilt = params.getInt("tilt", tilt);
  }
  tiltDegrees_ = std::clamp(tilt, -90, 90);

  if (params.contains("rotation")) {
    rotationDegrees_ = params.getFloat("rotation", rotationDegrees_);
  } else if (params.contains("phase")) {
    rotationDegrees_ = params.getFloat("phase", rotationDegrees_);
  }

  for (std::size_t i = 0; i < palette_.size(); ++i) {
    applyColorParam(params, "color", i);
    applyColorParam(params, "colour", i);
    applyColorParam(params, "color_", i);
  }

  if (paletteDirty_) {
    rebuildColorGradient();
  }
}

}  // namespace avs::effects::render
