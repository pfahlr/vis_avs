#include "avs/registry.hpp"
#include "avs/core.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>
#include <vector>

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
    const uint8_t* row = fb.data + static_cast<size_t>(y) * fb.stride;
    for (int x = 0; x < fb.width; ++x) {
      std::fputc(row[x * 4 + 0], f);
      std::fputc(row[x * 4 + 1], f);
      std::fputc(row[x * 4 + 2], f);
    }
  }
  std::fclose(f);
}

void generateAudio(AudioFeatures& af, int frameIdx, int sampleRate, int sampleCount, float frequency) {
  af.oscL.resize(sampleCount);
  af.oscR.resize(sampleCount);
  const double startTime = static_cast<double>(frameIdx) * static_cast<double>(sampleCount) /
                           static_cast<double>(sampleRate);
  for (int i = 0; i < sampleCount; ++i) {
    const double t = startTime + static_cast<double>(i) / static_cast<double>(sampleRate);
    const float sample = std::sin(2.0 * M_PI * static_cast<double>(frequency) * t);
    af.oscL[i] = sample;
    af.oscR[i] = sample;
  }
  af.beat = (frameIdx % 30) == 0;
  af.sample_rate = sampleRate;
}

}  // namespace

int main() {
  const int width = 640;
  const int height = 480;
  std::vector<uint8_t> bufferCurrent(static_cast<size_t>(width) * height * 4u, 0);
  std::vector<uint8_t> bufferPrevious(static_cast<size_t>(width) * height * 4u, 0);

  FrameBufferView fbCurrent{bufferCurrent.data(), width, height, width * 4};
  FrameBufferView fbPrevious{bufferPrevious.data(), width, height, width * 4};
  FrameBuffers frameBuffers{fbCurrent, fbPrevious};

  Registry registry;
  register_builtin_effects(registry);

  auto star = registry.create("starfield");
  auto osc = registry.create("oscilloscope");
  auto fade = registry.create("fadeout");
  auto clear = registry.create("clear_screen");

  if (!star || !osc || !fade || !clear) {
    std::fprintf(stderr, "Failed to create built-in effects.\n");
    return 1;
  }

  InitContext initCtx{{width, height}, {true, false, false, true}, true, 60};
  star->init(initCtx);
  osc->init(initCtx);
  fade->init(initCtx);
  clear->init(initCtx);

  TimingInfo timing;
  timing.deterministic = true;
  timing.fps_hint = 60;

  std::filesystem::create_directories("out");
  constexpr int frames = 120;
  constexpr int sampleRate = 44100;
  constexpr int hop = 1024;

  for (int frameIndex = 0; frameIndex < frames; ++frameIndex) {
    timing.frame_index = frameIndex;
    timing.t_seconds = static_cast<double>(frameIndex) * static_cast<double>(hop) /
                       static_cast<double>(sampleRate);
    timing.dt_seconds = static_cast<double>(hop) / static_cast<double>(sampleRate);

    std::memcpy(bufferPrevious.data(), bufferCurrent.data(), bufferCurrent.size());

    AudioFeatures audio;
    generateAudio(audio, frameIndex, sampleRate, hop, 220.0f);

    ProcessContext processCtx{timing, audio, frameBuffers, nullptr, nullptr};

    clear->process(processCtx, frameBuffers.current);
    star->process(processCtx, frameBuffers.current);
    osc->process(processCtx, frameBuffers.current);
    fade->process(processCtx, frameBuffers.current);

    char path[256];
    std::snprintf(path, sizeof(path), "out/frame_%04d.ppm", frameIndex);
    writePpm(path, frameBuffers.current);
  }

  return 0;
}
