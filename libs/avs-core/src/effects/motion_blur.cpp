#include <emmintrin.h>

#include <algorithm>

#include "avs/cpu_features.hpp"
#include "avs/effects.hpp"

namespace avs {

void MotionBlurEffect::init(int w, int h) {
  prev_.w = w;
  prev_.h = h;
  prev_.rgba.assign(static_cast<size_t>(w) * h * 4, 0);
}

void MotionBlurEffect::process(const Framebuffer& in, Framebuffer& out) {
  out.w = in.w;
  out.h = in.h;
  out.rgba.resize(in.rgba.size());
  if (prev_.rgba.size() != in.rgba.size()) {
    prev_.rgba.assign(in.rgba.size(), 0);
  }
  if (hasSse2()) {
    const std::uint8_t* a = in.rgba.data();
    const std::uint8_t* p = prev_.rgba.data();
    std::uint8_t* o = out.rgba.data();
    size_t n = in.rgba.size();
    size_t i = 0;
    for (; i + 16 <= n; i += 16) {
      __m128i va = _mm_loadu_si128(reinterpret_cast<const __m128i*>(a + i));
      __m128i vp = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p + i));
      __m128i v = _mm_avg_epu8(va, vp);
      _mm_storeu_si128(reinterpret_cast<__m128i*>(o + i), v);
    }
    for (; i < n; ++i) {
      o[i] = static_cast<std::uint8_t>((static_cast<int>(a[i]) + p[i]) / 2);
    }
  } else {
    for (size_t i = 0; i < in.rgba.size(); ++i) {
      out.rgba[i] = static_cast<std::uint8_t>((static_cast<int>(in.rgba[i]) + prev_.rgba[i]) / 2);
    }
  }
  prev_.rgba = out.rgba;
}

}  // namespace avs
