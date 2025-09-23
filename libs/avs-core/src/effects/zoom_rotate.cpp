#include <emmintrin.h>

#include <algorithm>

#include "avs/cpu_features.hpp"
#include "avs/effects.hpp"

namespace avs {

void ZoomRotateEffect::init(int w, int h) {
  (void)w;
  (void)h;
}

void ZoomRotateEffect::process(const Framebuffer& in, Framebuffer& out) {
  out.w = in.w;
  out.h = in.h;
  out.rgba.resize(in.rgba.size());
  const std::uint8_t* src = in.rgba.data();
  std::uint8_t* dst = out.rgba.data();
  size_t n = in.rgba.size();
  size_t pixels = n / 4;
  if (hasSse2()) {
    size_t i = 0;
    while (i + 4 <= pixels) {
      __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i_u*>(src + (pixels - i - 4) * 4));
      v = _mm_shuffle_epi32(v, _MM_SHUFFLE(0, 1, 2, 3));
      _mm_storeu_si128(reinterpret_cast<__m128i_u*>(dst + i * 4), v);
      i += 4;
    }
    for (; i < pixels; ++i) {
      const std::uint8_t* s = src + (pixels - i - 1) * 4;
      std::uint8_t* d = dst + i * 4;
      d[0] = s[0];
      d[1] = s[1];
      d[2] = s[2];
      d[3] = s[3];
    }
  } else {
    for (size_t i = 0; i < pixels; ++i) {
      const std::uint8_t* s = src + (pixels - i - 1) * 4;
      std::uint8_t* d = dst + i * 4;
      d[0] = s[0];
      d[1] = s[1];
      d[2] = s[2];
      d[3] = s[3];
    }
  }
}

}  // namespace avs
