#include <emmintrin.h>

#include <algorithm>

#include "avs/cpu_features.hpp"
#include "avs/effects.hpp"

namespace avs {

void ColorTransformEffect::init(int w, int h) {
  (void)w;
  (void)h;
}

void ColorTransformEffect::process(const Framebuffer& in, Framebuffer& out) {
  out.w = in.w;
  out.h = in.h;
  out.rgba.resize(in.rgba.size());
  const std::uint8_t* src = in.rgba.data();
  std::uint8_t* dst = out.rgba.data();
  size_t n = in.rgba.size();
  if (hasSse2()) {
    __m128i mask = _mm_set1_epi8(static_cast<char>(0xFF));
    size_t i = 0;
    for (; i + 16 <= n; i += 16) {
      __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i_u*>(src + i));
      v = _mm_sub_epi8(mask, v);
      _mm_storeu_si128(reinterpret_cast<__m128i_u*>(dst + i), v);
    }
    for (; i < n; ++i) {
      dst[i] = 255 - src[i];
    }
  } else {
    for (size_t i = 0; i < n; ++i) {
      dst[i] = 255 - src[i];
    }
  }
}

}  // namespace avs
