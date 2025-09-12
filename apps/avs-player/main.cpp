#include <chrono>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <vector>

#include "avs/audio.hpp"
#include "avs/effects.hpp"
#include "avs/engine.hpp"
#include "avs/window.hpp"

int main() {
  avs::Window window(1920, 1080, "AVS Player");
  avs::AudioInput audio;
  if (!audio.ok()) {
    std::fprintf(stderr, "audio init failed\n");
    return 1;
  }

  avs::Engine engine(1920, 1080);
  std::vector<std::unique_ptr<avs::Effect>> chain;
  chain.push_back(std::make_unique<avs::BlurEffect>());
  chain.push_back(std::make_unique<avs::ColorMapEffect>());
  chain.push_back(std::make_unique<avs::ConvolutionEffect>());
  engine.setChain(std::move(chain));

  auto last = std::chrono::steady_clock::now();
  float printAccum = 0.0f;
  while (window.poll()) {
    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - last).count();
    last = now;

    auto s = audio.poll();
    engine.setAudio(s);
    printAccum += dt;
    if (printAccum > 0.5f) {
      printAccum = 0.0f;
      std::printf("rms %.3f bands %.3f %.3f %.3f\n", s.rms, s.bands[0], s.bands[1], s.bands[2]);
    }

    auto [w, h] = window.size();
    engine.resize(w, h);
    engine.step(dt);
    window.blit(engine.frame().rgba.data(), w, h);
  }
  return 0;
}
