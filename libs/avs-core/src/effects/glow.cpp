#include <emmintrin.h>

#include <algorithm>

#include "avs/cpu_features.hpp"
#include "avs/effects.hpp"

namespace avs {

void GlowEffect::process(const Framebuffer& in, Framebuffer& out) {
  out.w = in.w;
  out.h = in.h;
  out.rgba.resize(in.rgba.size());
  const std::uint8_t* src = in.rgba.data();
  std::uint8_t* dst = out.rgba.data();
  size_t n = in.rgba.size();
  if (hasSse2()) {
    __m128i add = _mm_set1_epi8(50);
    size_t i = 0;
    for (; i + 16 <= n; i += 16) {
      __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i_u*>(src + i));
      v = _mm_adds_epu8(v, add);
      _mm_storeu_si128(reinterpret_cast<__m128i_u*>(dst + i), v);
    }
    for (; i < n; ++i) {
      int v = src[i] + 50;
      dst[i] = static_cast<std::uint8_t>(v > 255 ? 255 : v);
    }
  } else {
    for (size_t i = 0; i < n; ++i) {
      int v = src[i] + 50;
      dst[i] = static_cast<std::uint8_t>(v > 255 ? 255 : v);
    }
  }
}

}  // namespace avs
