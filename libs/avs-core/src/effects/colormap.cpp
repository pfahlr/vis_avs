#include <array>
#include <cmath>

#include "avs/effects.hpp"

namespace avs {

static std::array<std::uint8_t, 3> hsvToRgb(float h) {
  float s = 1.0f;
  float v = 1.0f;
  float c = v * s;
  float x = c * (1 - std::fabs(std::fmod(h / 60.0f, 2.0f) - 1));
  float m = v - c;
  float r = 0, g = 0, b = 0;
  if (h < 60) {
    r = c;
    g = x;
  } else if (h < 120) {
    r = x;
    g = c;
  } else if (h < 180) {
    g = c;
    b = x;
  } else if (h < 240) {
    g = x;
    b = c;
  } else if (h < 300) {
    r = x;
    b = c;
  } else {
    r = c;
    b = x;
  }
  return {static_cast<std::uint8_t>((r + m) * 255), static_cast<std::uint8_t>((g + m) * 255),
          static_cast<std::uint8_t>((b + m) * 255)};
}

void ColorMapEffect::init(int /*w*/, int /*h*/) {
  for (int i = 0; i < 256; ++i) {
    float t = static_cast<float>(i) / 255.0f;
    auto rgb = hsvToRgb(t * 360.0f);
    lut_[i * 3 + 0] = rgb[0];
    lut_[i * 3 + 1] = rgb[1];
    lut_[i * 3 + 2] = rgb[2];
  }
}

void ColorMapEffect::process(const Framebuffer& in, Framebuffer& out) {
  out.w = in.w;
  out.h = in.h;
  out.rgba.resize(in.rgba.size());
  int w = in.w;
  int h = in.h;
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      size_t idx = (static_cast<size_t>(y) * w + x) * 4;
      std::uint8_t g = in.rgba[idx];
      out.rgba[idx + 0] = lut_[g * 3 + 0];
      out.rgba[idx + 1] = lut_[g * 3 + 1];
      out.rgba[idx + 2] = lut_[g * 3 + 2];
      out.rgba[idx + 3] = 255;
    }
  }
}

}  // namespace avs
