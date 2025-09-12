#include <chrono>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include "avs/audio.hpp"
#include "avs/effects.hpp"
#include "avs/engine.hpp"
#include "avs/window.hpp"

int main(int argc, char** argv) {
  bool demoScript = false;
  for (int i = 1; i < argc; ++i) {
    if (std::string(argv[i]) == "--demo-script") demoScript = true;
  }

  avs::Window window(1920, 1080, "AVS Player");
  avs::AudioInput audio;
  if (!audio.ok()) {
    std::fprintf(stderr, "audio init failed\n");
    return 1;
  }

  avs::Engine engine(1920, 1080);
  std::vector<std::unique_ptr<avs::Effect>> chain;
  if (demoScript) {
    std::string frameScript;
    std::string pixelScript =
        "red = clamp(sin(x*0.01 + time)*bass,0,1);"
        "green = clamp(sin(y*0.01 + time)*mid,0,1);"
        "blue = clamp(sin((x+y)*0.01 + time)*treb,0,1);";
    chain.push_back(std::make_unique<avs::ScriptedEffect>(frameScript, pixelScript));
  } else {
    chain.push_back(std::make_unique<avs::BlurEffect>());
    chain.push_back(std::make_unique<avs::ColorMapEffect>());
    chain.push_back(std::make_unique<avs::ConvolutionEffect>());
  }
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
