#include <emmintrin.h>

#include <algorithm>

#include "avs/cpu_features.hpp"
#include "avs/effects.hpp"

namespace avs {

void ConvolutionEffect::init(int /*w*/, int /*h*/) { kernel_ = {0, -1, 0, -1, 5, -1, 0, -1, 0}; }

namespace {
void convScalar(const Framebuffer& in, Framebuffer& out, const std::array<int, 9>& kernel) {
  int w = in.w;
  int h = in.h;
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      float r = 0, g = 0, b = 0;
      for (int ky = -1; ky <= 1; ++ky) {
        for (int kx = -1; kx <= 1; ++kx) {
          int ix = std::clamp(x + kx, 0, w - 1);
          int iy = std::clamp(y + ky, 0, h - 1);
          size_t idx = (static_cast<size_t>(iy) * w + ix) * 4;
          int k = kernel[static_cast<size_t>(ky + 1) * 3 + (kx + 1)];
          r += k * in.rgba[idx + 0];
          g += k * in.rgba[idx + 1];
          b += k * in.rgba[idx + 2];
        }
      }
      size_t outIdx = (static_cast<size_t>(y) * w + x) * 4;
      auto clamp8 = [](float v) { return static_cast<std::uint8_t>(std::clamp(v, 0.0f, 255.0f)); };
      out.rgba[outIdx + 0] = clamp8(r);
      out.rgba[outIdx + 1] = clamp8(g);
      out.rgba[outIdx + 2] = clamp8(b);
      out.rgba[outIdx + 3] = in.rgba[outIdx + 3];
    }
  }
}

void convSse2(const Framebuffer& in, Framebuffer& out, const std::array<int, 9>& kernel) {
  int w = in.w;
  int h = in.h;
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      __m128 accum = _mm_setzero_ps();
      for (int ky = -1; ky <= 1; ++ky) {
        for (int kx = -1; kx <= 1; ++kx) {
          int ix = std::clamp(x + kx, 0, w - 1);
          int iy = std::clamp(y + ky, 0, h - 1);
          size_t idx = (static_cast<size_t>(iy) * w + ix) * 4;
          int k = kernel[static_cast<size_t>(ky + 1) * 3 + (kx + 1)];
          __m128 pix = _mm_set_ps(0.0f, in.rgba[idx + 2], in.rgba[idx + 1], in.rgba[idx + 0]);
          __m128 wgt = _mm_set1_ps(static_cast<float>(k));
          accum = _mm_add_ps(accum, _mm_mul_ps(pix, wgt));
        }
      }
      alignas(16) float tmp[4];
      _mm_store_ps(tmp, accum);
      size_t outIdx = (static_cast<size_t>(y) * w + x) * 4;
      auto clamp8 = [](float v) { return static_cast<std::uint8_t>(std::clamp(v, 0.0f, 255.0f)); };
      out.rgba[outIdx + 0] = clamp8(tmp[0]);
      out.rgba[outIdx + 1] = clamp8(tmp[1]);
      out.rgba[outIdx + 2] = clamp8(tmp[2]);
      out.rgba[outIdx + 3] = in.rgba[outIdx + 3];
    }
  }
}
}  // namespace

void ConvolutionEffect::process(const Framebuffer& in, Framebuffer& out) {
  out.w = in.w;
  out.h = in.h;
  out.rgba.resize(in.rgba.size());
  if (hasSse2()) {
    convSse2(in, out, kernel_);
  } else {
    convScalar(in, out, kernel_);
  }
}

}  // namespace avs
