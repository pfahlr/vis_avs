#pragma once

#include <vector>

namespace avs {

// Simple real FFT wrapper producing magnitude spectrum using KissFFT.
class FFT {
 public:
  explicit FFT(int n);
  ~FFT();

  int size() const { return n_; }

  // Compute magnitude spectrum of real input of length n().
  // Output vector is resized to n()/2.
  void compute(const float* input, std::vector<float>& out);

 private:
  int n_ = 0;
  struct Impl;
  Impl* impl_ = nullptr;
};

}  // namespace avs
