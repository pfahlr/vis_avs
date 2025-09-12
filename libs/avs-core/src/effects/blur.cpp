#include <emmintrin.h>

#include <algorithm>
#include <cmath>

#include "avs/cpu_features.hpp"
#include "avs/effects.hpp"

namespace avs {

BlurEffect::BlurEffect(int radius) : radius_(radius) {}

void BlurEffect::init(int w, int h) {
  temp_.w = w;
  temp_.h = h;
  temp_.rgba.resize(static_cast<size_t>(w) * h * 4);
  int size = radius_ * 2 + 1;
  kernel_.resize(static_cast<size_t>(size));
  float sigma = radius_ / 2.0f;
  float sum = 0.0f;
  for (int i = 0; i < size; ++i) {
    int x = i - radius_;
    float v = std::exp(-(x * x) / (2.0f * sigma * sigma));
    kernel_[static_cast<size_t>(i)] = v;
    sum += v;
  }
  for (auto& k : kernel_) {
    k /= sum;
  }
}

namespace {
void blurScalar(const Framebuffer& in, Framebuffer& temp, Framebuffer& out,
                const std::vector<float>& kernel, int radius) {
  int w = in.w;
  int h = in.h;
  int size = radius * 2 + 1;

  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      float r = 0, g = 0, b = 0, a = 0;
      for (int k = 0; k < size; ++k) {
        int ix = std::clamp(x + k - radius, 0, w - 1);
        size_t idx = (static_cast<size_t>(y) * w + ix) * 4;
        float wgt = kernel[static_cast<size_t>(k)];
        r += wgt * in.rgba[idx + 0];
        g += wgt * in.rgba[idx + 1];
        b += wgt * in.rgba[idx + 2];
        a += wgt * in.rgba[idx + 3];
      }
      size_t outIdx = (static_cast<size_t>(y) * w + x) * 4;
      temp.rgba[outIdx + 0] = static_cast<std::uint8_t>(r);
      temp.rgba[outIdx + 1] = static_cast<std::uint8_t>(g);
      temp.rgba[outIdx + 2] = static_cast<std::uint8_t>(b);
      temp.rgba[outIdx + 3] = static_cast<std::uint8_t>(a);
    }
  }

  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      float r = 0, g = 0, b = 0, a = 0;
      for (int k = 0; k < size; ++k) {
        int iy = std::clamp(y + k - radius, 0, h - 1);
        size_t idx = (static_cast<size_t>(iy) * w + x) * 4;
        float wgt = kernel[static_cast<size_t>(k)];
        r += wgt * temp.rgba[idx + 0];
        g += wgt * temp.rgba[idx + 1];
        b += wgt * temp.rgba[idx + 2];
        a += wgt * temp.rgba[idx + 3];
      }
      size_t outIdx = (static_cast<size_t>(y) * w + x) * 4;
      out.rgba[outIdx + 0] = static_cast<std::uint8_t>(r);
      out.rgba[outIdx + 1] = static_cast<std::uint8_t>(g);
      out.rgba[outIdx + 2] = static_cast<std::uint8_t>(b);
      out.rgba[outIdx + 3] = static_cast<std::uint8_t>(a);
    }
  }
}

void blurSse2(const Framebuffer& in, Framebuffer& temp, Framebuffer& out,
              const std::vector<float>& kernel, int radius) {
  int w = in.w;
  int h = in.h;
  int size = radius * 2 + 1;

  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      __m128 accum = _mm_setzero_ps();
      for (int k = 0; k < size; ++k) {
        int ix = std::clamp(x + k - radius, 0, w - 1);
        size_t idx = (static_cast<size_t>(y) * w + ix) * 4;
        __m128 pix =
            _mm_set_ps(in.rgba[idx + 3], in.rgba[idx + 2], in.rgba[idx + 1], in.rgba[idx + 0]);
        __m128 wgt = _mm_set1_ps(kernel[static_cast<size_t>(k)]);
        accum = _mm_add_ps(accum, _mm_mul_ps(wgt, pix));
      }
      alignas(16) float tmp[4];
      _mm_store_ps(tmp, accum);
      size_t outIdx = (static_cast<size_t>(y) * w + x) * 4;
      temp.rgba[outIdx + 0] = static_cast<std::uint8_t>(tmp[0]);
      temp.rgba[outIdx + 1] = static_cast<std::uint8_t>(tmp[1]);
      temp.rgba[outIdx + 2] = static_cast<std::uint8_t>(tmp[2]);
      temp.rgba[outIdx + 3] = static_cast<std::uint8_t>(tmp[3]);
    }
  }

  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      __m128 accum = _mm_setzero_ps();
      for (int k = 0; k < size; ++k) {
        int iy = std::clamp(y + k - radius, 0, h - 1);
        size_t idx = (static_cast<size_t>(iy) * w + x) * 4;
        __m128 pix = _mm_set_ps(temp.rgba[idx + 3], temp.rgba[idx + 2], temp.rgba[idx + 1],
                                temp.rgba[idx + 0]);
        __m128 wgt = _mm_set1_ps(kernel[static_cast<size_t>(k)]);
        accum = _mm_add_ps(accum, _mm_mul_ps(wgt, pix));
      }
      alignas(16) float tmp[4];
      _mm_store_ps(tmp, accum);
      size_t outIdx = (static_cast<size_t>(y) * w + x) * 4;
      out.rgba[outIdx + 0] = static_cast<std::uint8_t>(tmp[0]);
      out.rgba[outIdx + 1] = static_cast<std::uint8_t>(tmp[1]);
      out.rgba[outIdx + 2] = static_cast<std::uint8_t>(tmp[2]);
      out.rgba[outIdx + 3] = static_cast<std::uint8_t>(tmp[3]);
    }
  }
}
}  // namespace

void BlurEffect::process(const Framebuffer& in, Framebuffer& out) {
  temp_.w = in.w;
  temp_.h = in.h;
  out.w = in.w;
  out.h = in.h;
  temp_.rgba.resize(in.rgba.size());
  out.rgba.resize(in.rgba.size());
  if (hasSse2()) {
    blurSse2(in, temp_, out, kernel_, radius_);
  } else {
    blurScalar(in, temp_, out, kernel_, radius_);
  }
}

}  // namespace avs
