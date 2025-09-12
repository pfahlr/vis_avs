#include <emmintrin.h>

#include <algorithm>
#include <cmath>

#include "avs/cpu_features.hpp"
#include "avs/effects.hpp"

namespace avs {

void TunnelEffect::init(int w, int h) {
  cx_ = w / 2;
  cy_ = h / 2;
}

void TunnelEffect::process(const Framebuffer& in, Framebuffer& out) {
  (void)in;
  out.w = in.w;
  out.h = in.h;
  out.rgba.resize(static_cast<size_t>(out.w) * out.h * 4);
  int w = out.w;
  int h = out.h;
  if (hasSse2()) {
    for (int y = 0; y < h; ++y) {
      __m128 dy = _mm_set1_ps(static_cast<float>(y - cy_));
      for (int x = 0; x < w; x += 4) {
        __m128 dx = _mm_set_ps(static_cast<float>(x + 3 - cx_), static_cast<float>(x + 2 - cx_),
                               static_cast<float>(x + 1 - cx_), static_cast<float>(x - cx_));
        __m128 dist = _mm_sqrt_ps(_mm_add_ps(_mm_mul_ps(dx, dx), _mm_mul_ps(dy, dy)));
        float tmp[4];
        _mm_storeu_ps(tmp, dist);
        for (int i = 0; i < 4 && x + i < w; ++i) {
          int v = static_cast<int>(tmp[3 - i]);
          v = std::clamp(v, 0, 255);
          size_t idx = (static_cast<size_t>(y) * w + x + i) * 4;
          out.rgba[idx + 0] = static_cast<std::uint8_t>(v);
          out.rgba[idx + 1] = static_cast<std::uint8_t>(v);
          out.rgba[idx + 2] = static_cast<std::uint8_t>(v);
          out.rgba[idx + 3] = 255;
        }
      }
    }
  } else {
    for (int y = 0; y < h; ++y) {
      for (int x = 0; x < w; ++x) {
        int dx = x - cx_;
        int dy = y - cy_;
        int v = static_cast<int>(std::sqrt(dx * dx + dy * dy));
        v = std::clamp(v, 0, 255);
        size_t idx = (static_cast<size_t>(y) * w + x) * 4;
        out.rgba[idx + 0] = static_cast<std::uint8_t>(v);
        out.rgba[idx + 1] = static_cast<std::uint8_t>(v);
        out.rgba[idx + 2] = static_cast<std::uint8_t>(v);
        out.rgba[idx + 3] = 255;
      }
    }
  }
}

}  // namespace avs
