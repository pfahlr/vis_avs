#include <cmath>
#include <cstdlib>

#include "avs/fft.hpp"

extern "C" {
#include "kiss_fft.h"
#include "kiss_fftr.h"
}

namespace avs {

struct FFT::Impl {
  kiss_fftr_cfg cfg = nullptr;
  std::vector<kiss_fft_cpx> freq;  // n/2+1
};

FFT::FFT(int n) : n_(n), impl_(new Impl) {
  impl_->cfg = kiss_fftr_alloc(n_, 0, nullptr, nullptr);
  impl_->freq.resize(n_ / 2 + 1);
}

FFT::~FFT() {
  kiss_fft_free(impl_->cfg);
  delete impl_;
}

void FFT::compute(const float* input, std::vector<float>& out) {
  kiss_fftr(impl_->cfg, input, impl_->freq.data());
  size_t bins = static_cast<size_t>(n_) / 2;
  out.resize(bins);
  for (size_t i = 0; i < bins; ++i) {
    auto c = impl_->freq[i];
    float mag = std::sqrt(c.r * c.r + c.i * c.i);
    out[i] = mag / static_cast<float>(bins);
  }
}

}  // namespace avs
