#pragma once

#include <array>
#include <vector>

namespace avs {

struct AudioState {
  float rms = 0.0f;                             // 0..1
  std::array<float, 3> bands{{0.f, 0.f, 0.f}};  // bass, mid, treble smoothed
  std::vector<float> spectrum;                  // N/2 magnitudes [0,1]
  double timeSeconds = 0.0;                     // audio clock
};

class AudioInput {
 public:
  AudioInput(int sampleRate = 48000, int channels = 2);
  ~AudioInput();

  bool ok() const { return ok_; }

  // Poll produces a snapshot computed from internal ring buffer.
  AudioState poll();

 private:
  struct Impl;
  Impl* impl_ = nullptr;
  bool ok_ = false;
};

}  // namespace avs
