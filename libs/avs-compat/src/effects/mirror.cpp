#include <emmintrin.h>

#include <algorithm>

#include <avs/cpu_features.hpp>
#include <avs/effects.hpp>

namespace avs {

void MirrorEffect::init(int w, int h) {
  (void)w;
  (void)h;
}

void MirrorEffect::process(const Framebuffer& in, Framebuffer& out) {
  out.w = in.w;
  out.h = in.h;
  out.rgba.resize(in.rgba.size());
  int w = in.w;
  int h = in.h;
  const std::uint8_t* src = in.rgba.data();
  std::uint8_t* dst = out.rgba.data();
  size_t rowPixels = static_cast<size_t>(w);
  if (hasSse2()) {
    for (int y = 0; y < h; ++y) {
      const std::uint8_t* s = src + static_cast<size_t>(y) * rowPixels * 4;
      std::uint8_t* d = dst + static_cast<size_t>(y) * rowPixels * 4;
      size_t i = 0;
      while (i + 4 <= rowPixels) {
        __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i_u*>(s + (rowPixels - i - 4) * 4));
        v = _mm_shuffle_epi32(v, _MM_SHUFFLE(0, 1, 2, 3));
        _mm_storeu_si128(reinterpret_cast<__m128i_u*>(d + i * 4), v);
        i += 4;
      }
      for (; i < rowPixels; ++i) {
        const std::uint8_t* sp = s + (rowPixels - i - 1) * 4;
        std::uint8_t* dp = d + i * 4;
        dp[0] = sp[0];
        dp[1] = sp[1];
        dp[2] = sp[2];
        dp[3] = sp[3];
      }
    }
  } else {
    for (int y = 0; y < h; ++y) {
      const std::uint8_t* s = src + static_cast<size_t>(y) * rowPixels * 4;
      std::uint8_t* d = dst + static_cast<size_t>(y) * rowPixels * 4;
      for (size_t x = 0; x < rowPixels; ++x) {
        const std::uint8_t* sp = s + (rowPixels - x - 1) * 4;
        std::uint8_t* dp = d + x * 4;
        dp[0] = sp[0];
        dp[1] = sp[1];
        dp[2] = sp[2];
        dp[3] = sp[3];
      }
    }
  }
}

}  // namespace avs
