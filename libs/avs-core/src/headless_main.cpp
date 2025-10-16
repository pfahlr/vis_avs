#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <vector>

#include "avs/core.hpp"
#include "avs/registry.hpp"

using namespace avs;

namespace {

void writePpm(const char* path, const FrameBufferView& fb) {
  FILE* f = std::fopen(path, "wb");
  if (!f) {
    std::perror("fopen");
    return;
  }
  std::fprintf(f, "P6\n%d %d\n255\n", fb.width, fb.height);
  for (int y = 0; y < fb.height; ++y) {
    const uint8_t* row = fb.data + y * fb.stride;
    for (int x = 0; x < fb.width; ++x) {
      std::fputc(static_cast<int>(row[x * 4 + 0]), f);
      std::fputc(static_cast<int>(row[x * 4 + 1]), f);
      std::fputc(static_cast<int>(row[x * 4 + 2]), f);
    }
  }
  std::fclose(f);
}

void genAudio(AudioFeatures& af, int frameIdx, int sampleRate, int sampleCount, float freq) {
  af.oscL.resize(sampleCount);
  af.oscR.resize(sampleCount);
  const double baseT =
      frameIdx * static_cast<double>(sampleCount) / static_cast<double>(sampleRate);
  constexpr double kTwoPi = 6.28318530717958647692;
  for (int i = 0; i < sampleCount; ++i) {
    double t = baseT + static_cast<double>(i) / static_cast<double>(sampleRate);
    float s = std::sin(kTwoPi * freq * t);
    af.oscL[i] = s;
    af.oscR[i] = s;
  }
  af.beat = (frameIdx % 30) == 0;
  af.sample_rate = sampleRate;
}

}  // namespace

int main() {
  const int width = 640;
  const int height = 480;
  std::vector<uint8_t> bufCur(width * height * 4, 0);
  std::vector<uint8_t> bufPrev(width * height * 4, 0);
  FrameBufferView fbCur{bufCur.data(), width, height, width * 4};
  FrameBufferView fbPrev{bufPrev.data(), width, height, width * 4};
  FrameBuffers frameBuffers{fbCur, fbPrev};

  Registry registry;
  register_builtin_effects(registry);

  auto clear = registry.create("clear_screen");
  auto star = registry.create("starfield");
  auto osc = registry.create("oscilloscope");
  auto fade = registry.create("fadeout");
  if (!clear || !star || !osc || !fade) {
    std::fprintf(stderr, "Failed to create required effects\n");
    return 1;
  }

  InitContext initCtx{{width, height}, {true, false, false, true}, true, 60};
  clear->init(initCtx);
  star->init(initCtx);
  osc->init(initCtx);
  fade->init(initCtx);

  TimingInfo timing;
  timing.deterministic = true;
  timing.fps_hint = 60;

  std::filesystem::create_directories("out");
  const int totalFrames = 120;
  const int sampleRate = 44100;
  const int hop = 1024;

  for (int fi = 0; fi < totalFrames; ++fi) {
    timing.frame_index = fi;
    timing.t_seconds = fi * static_cast<double>(hop) / static_cast<double>(sampleRate);
    timing.dt_seconds = static_cast<double>(hop) / static_cast<double>(sampleRate);

    std::memcpy(bufPrev.data(), bufCur.data(), bufCur.size());

    AudioFeatures audio;
    genAudio(audio, fi, sampleRate, hop, 220.0f);

    ProcessContext processCtx{timing, audio, frameBuffers, nullptr, nullptr};

    clear->process(processCtx, frameBuffers.current);
    star->process(processCtx, frameBuffers.current);
    osc->process(processCtx, frameBuffers.current);
    fade->process(processCtx, frameBuffers.current);

    char path[256];
    std::snprintf(path, sizeof(path), "out/frame_%04d.ppm", fi);
    writePpm(path, frameBuffers.current);
  }

  return 0;
}
