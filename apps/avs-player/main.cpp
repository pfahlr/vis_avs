#include <chrono>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "avs/audio.hpp"
#include "avs/window.hpp"

int main() {
  avs::Window window(1920, 1080, "AVS Player");
  avs::AudioInput audio;
  if (!audio.ok()) {
    std::fprintf(stderr, "audio init failed\n");
    return 1;
  }

  std::vector<std::uint8_t> fb;
  auto last = std::chrono::steady_clock::now();
  float t = 0.0f;
  float printAccum = 0.0f;
  while (window.poll()) {
    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - last).count();
    last = now;
    t += dt;
    printAccum += dt;
    if (printAccum > 0.5f) {
      printAccum = 0.0f;
      auto s = audio.poll();
      std::printf("rms %.3f bands %.3f %.3f %.3f\n", s.rms, s.bands[0], s.bands[1], s.bands[2]);
    }

    auto [w, h] = window.size();
    fb.resize(static_cast<size_t>(w) * h * 4);
    for (int y = 0; y < h; ++y) {
      for (int x = 0; x < w; ++x) {
        int idx = (y * w + x) * 4;
        fb[idx + 0] = static_cast<std::uint8_t>(x + t * 100);
        fb[idx + 1] = static_cast<std::uint8_t>(y + t * 100);
        fb[idx + 2] = static_cast<std::uint8_t>((x + y) + t * 100);
        fb[idx + 3] = 255;
      }
    }
    window.blit(fb.data(), w, h);
  }
  return 0;
}
