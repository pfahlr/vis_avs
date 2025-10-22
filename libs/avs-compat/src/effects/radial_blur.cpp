#include <emmintrin.h>

#include <algorithm>
#include <cstring>

#include <avs/cpu_features.hpp>
#include <avs/effects.hpp>

namespace avs {

void RadialBlurEffect::init(int w, int h) {
  (void)w;
  (void)h;
}

void RadialBlurEffect::process(const Framebuffer& in, Framebuffer& out) {
  out.w = in.w;
  out.h = in.h;
  out.rgba.resize(in.rgba.size());
  int w = in.w;
  int h = in.h;
  size_t centerIdx = (static_cast<size_t>(h / 2) * w + (w / 2)) * 4;
  const std::uint8_t* center = in.rgba.data() + centerIdx;
  const std::uint8_t* src = in.rgba.data();
  std::uint8_t* dst = out.rgba.data();
  size_t n = in.rgba.size();
  if (hasSse2()) {
    std::uint32_t centerValue = 0;
    std::memcpy(&centerValue, center, sizeof(centerValue));
    __m128i c = _mm_set1_epi32(static_cast<int>(centerValue));
    size_t i = 0;
    for (; i + 16 <= n; i += 16) {
      __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i_u*>(src + i));
      __m128i r = _mm_avg_epu8(v, c);
      _mm_storeu_si128(reinterpret_cast<__m128i_u*>(dst + i), r);
    }
    for (; i < n; ++i) {
      dst[i] = static_cast<std::uint8_t>((static_cast<int>(src[i]) + center[i % 4]) / 2);
    }
  } else {
    for (size_t i = 0; i < n; ++i) {
      dst[i] = static_cast<std::uint8_t>((static_cast<int>(src[i]) + center[i % 4]) / 2);
    }
  }
}

}  // namespace avs
