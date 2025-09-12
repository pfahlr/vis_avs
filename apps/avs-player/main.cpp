#include <chrono>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "avs/audio.hpp"
#include "avs/effects.hpp"
#include "avs/engine.hpp"
#include "avs/fs.hpp"
#include "avs/preset.hpp"
#include "avs/window.hpp"

int main(int argc, char** argv) {
  bool demoScript = false;
  std::filesystem::path presetDir;
  for (int i = 1; i < argc; ++i) {
    if (std::string(argv[i]) == "--demo-script") {
      demoScript = true;
    } else if (std::string(argv[i]) == "--presets" && i + 1 < argc) {
      presetDir = argv[++i];
    }
  }

  avs::Window window(1920, 1080, "AVS Player");
  avs::AudioInput audio;
  if (!audio.ok()) {
    std::fprintf(stderr, "audio init failed\n");
    return 1;
  }

  avs::Engine engine(1920, 1080);
  std::vector<std::unique_ptr<avs::Effect>> chain;

  std::filesystem::path currentPreset;
  std::unique_ptr<avs::FileWatcher> watcher;
  auto loadPreset = [&]() {
    if (currentPreset.empty()) return;
    auto parsed = avs::parsePreset(currentPreset);
    if (!parsed.warnings.empty()) {
      for (const auto& w : parsed.warnings) {
        std::fprintf(stderr, "%s\n", w.c_str());
      }
    }
    engine.setChain(std::move(parsed.chain));
    watcher = std::make_unique<avs::FileWatcher>(currentPreset);
  };

  if (!presetDir.empty()) {
    for (auto& e : std::filesystem::directory_iterator(presetDir)) {
      if (e.is_regular_file() && e.path().extension() == ".avs") {
        currentPreset = e.path();
        break;
      }
    }
    loadPreset();
  } else if (demoScript) {
    std::string frameScript;
    std::string pixelScript =
        "red = clamp(sin(x*0.01 + time)*bass,0,1);"
        "green = clamp(sin(y*0.01 + time)*mid,0,1);"
        "blue = clamp(sin((x+y)*0.01 + time)*treb,0,1);";
    chain.push_back(std::make_unique<avs::ScriptedEffect>(frameScript, pixelScript));
    engine.setChain(std::move(chain));
  } else {
    chain.push_back(std::make_unique<avs::BlurEffect>());
    chain.push_back(std::make_unique<avs::ColorMapEffect>());
    chain.push_back(std::make_unique<avs::ConvolutionEffect>());
    engine.setChain(std::move(chain));
  }

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

    if (!currentPreset.empty()) {
      if (window.keyPressed('r') || (watcher && watcher->poll())) {
        loadPreset();
      }
    }

    auto [w, h] = window.size();
    engine.resize(w, h);
    engine.step(dt);
    window.blit(engine.frame().rgba.data(), w, h);
  }
  return 0;
}
