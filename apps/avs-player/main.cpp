#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#pragma GCC diagnostic pop
extern "C" {
#include "sha256.h"
}

#include "avs/audio.hpp"
#include "avs/effects.hpp"
#include "avs/engine.hpp"
#include "avs/fft.hpp"
#include "avs/fs.hpp"
#include "avs/preset.hpp"
#include "avs/window.hpp"

namespace {

struct WavData {
  std::vector<float> samples;
  unsigned int sampleRate = 0;
  unsigned int channels = 0;
};

bool loadWav(const std::filesystem::path& path, WavData& out) {
  drwav wav;
  if (!drwav_init_file(&wav, path.string().c_str(), nullptr)) {
    return false;
  }
  out.sampleRate = wav.sampleRate;
  out.channels = wav.channels;
  size_t totalFrames = wav.totalPCMFrameCount;
  out.samples.resize(totalFrames * wav.channels);
  drwav_read_pcm_frames_f32(&wav, totalFrames, out.samples.data());
  drwav_uninit(&wav);
  return true;
}

std::string hashFrame(const std::vector<std::uint8_t>& data) {
  SHA256_CTX ctx;
  sha256_init(&ctx);
  sha256_update(&ctx, data.data(), data.size());
  uint8_t hash[32];
  sha256_final(&ctx, hash);
  std::ostringstream oss;
  oss << std::hex << std::setfill('0');
  for (int i = 0; i < 32; ++i) {
    oss << std::setw(2) << static_cast<int>(hash[i]);
  }
  return oss.str();
}

void printUsage() {
  std::fprintf(
      stderr,
      "Usage: avs-player [--headless --wav <file> --preset <file> --frames <n> --out <dir>]\n"
      "                 [--sample-rate <hz>] [--channels <count>] [--demo-script]\n"
      "                 [--presets <directory>] [--help]\n");
}

class OfflineAudio {
 public:
  explicit OfflineAudio(const WavData& wav)
      : wav_(wav), fft_(kFftSize), mono_(kFftSize), spectrum_(kFftSize / 2) {}

  avs::AudioState poll() {
    avs::AudioState state;
    const size_t step = (wav_.sampleRate / 60) * wav_.channels;
    const size_t needed = kFftSize * wav_.channels;
    size_t w = pos_ + step;
    pos_ = w;

    long start = static_cast<long>(w) - static_cast<long>(needed);
    for (int i = 0; i < kFftSize; ++i) {
      float sum = 0.0f;
      for (unsigned int c = 0; c < wav_.channels; ++c) {
        long idx = start + static_cast<long>(i) * wav_.channels + c;
        float sample = 0.0f;
        if (idx >= 0 && idx < static_cast<long>(wav_.samples.size())) {
          sample = wav_.samples[static_cast<size_t>(idx)];
        }
        sum += sample;
      }
      mono_[i] = sum / static_cast<float>(wav_.channels);
    }
    float sumSq = 0.0f;
    for (float v : mono_) sumSq += v * v;
    state.rms = std::sqrt(sumSq / kFftSize);

    fft_.compute(mono_.data(), spectrum_);
    state.spectrum = spectrum_;

    std::array<float, 3> newBands{0.f, 0.f, 0.f};
    std::array<int, 3> counts{0, 0, 0};
    const double binHz = static_cast<double>(wav_.sampleRate) / kFftSize;
    for (size_t i = 0; i < spectrum_.size(); ++i) {
      double freq = i * binHz;
      float mag = spectrum_[i];
      if (freq < 250.0) {
        newBands[0] += mag;
        counts[0]++;
      } else if (freq < 4000.0) {
        newBands[1] += mag;
        counts[1]++;
      } else {
        newBands[2] += mag;
        counts[2]++;
      }
    }
    for (int i = 0; i < 3; ++i) {
      if (counts[i] > 0) newBands[i] /= static_cast<float>(counts[i]);
      bands_[i] = bands_[i] * (1.0f - kBandSmooth) + newBands[i] * kBandSmooth;
    }
    state.bands = bands_;
    state.timeSeconds =
        static_cast<double>(w) / (static_cast<double>(wav_.channels) * wav_.sampleRate);
    state.sampleRate = static_cast<int>(wav_.sampleRate);
    state.inputSampleRate = static_cast<int>(wav_.sampleRate);
    state.channels = static_cast<int>(wav_.channels);
    return state;
  }

 private:
  const WavData& wav_;
  size_t pos_ = 0;
  avs::FFT fft_;
  std::vector<float> mono_;
  std::vector<float> spectrum_;
  std::array<float, 3> bands_{{0.f, 0.f, 0.f}};
  static constexpr int kFftSize = 2048;
  static constexpr float kBandSmooth = 0.2f;
};

int runHeadless(const std::filesystem::path& wavPath, const std::filesystem::path& presetPath,
                int frames, const std::filesystem::path& outDir, bool writePngs) {
  WavData wav;
  if (!loadWav(wavPath, wav)) {
    std::fprintf(stderr, "failed to load wav\n");
    return 1;
  }
  if (wav.sampleRate == 0 || wav.channels == 0) {
    std::fprintf(stderr, "invalid wav\n");
    return 1;
  }

  auto parsed = avs::parsePreset(presetPath);
  if (!parsed.warnings.empty()) {
    for (const auto& w : parsed.warnings) {
      std::fprintf(stderr, "%s\n", w.c_str());
    }
  }

  const int width = 64;
  const int height = 64;
  avs::Engine engine(width, height);
  engine.setChain(std::move(parsed.chain));

  OfflineAudio audio(wav);

  std::filesystem::create_directories(outDir);
  std::ofstream hashes(outDir / "hashes.txt");
  if (!hashes) {
    std::fprintf(stderr, "failed to open hashes.txt\n");
    return 1;
  }

  for (int i = 0; i < frames; ++i) {
    auto s = audio.poll();
    engine.setAudio(s);
    engine.step(1.0f / 60.0f);
    const auto& fb = engine.frame();
    hashes << hashFrame(fb.rgba) << '\n';
    if (writePngs) {
      char name[32];
      std::snprintf(name, sizeof(name), "frame_%05d.png", i);
      std::filesystem::path pngPath = outDir / name;
      stbi_write_png(pngPath.string().c_str(), fb.w, fb.h, 4, fb.rgba.data(), fb.w * 4);
    }
  }
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  bool headless = false;
  std::filesystem::path wavPath;
  std::filesystem::path presetPath;
  std::filesystem::path outPath = ".";
  int frames = 0;
  bool demoScript = false;
  std::filesystem::path presetDir;
  bool showHelp = false;
  std::optional<int> requestedSampleRate;
  std::optional<int> requestedChannels;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--headless") {
      headless = true;
    } else if (arg == "--wav" && i + 1 < argc) {
      wavPath = argv[++i];
    } else if (arg == "--preset" && i + 1 < argc) {
      presetPath = argv[++i];
    } else if (arg == "--frames" && i + 1 < argc) {
      frames = std::stoi(argv[++i]);
    } else if (arg == "--out" && i + 1 < argc) {
      outPath = argv[++i];
    } else if (arg == "--demo-script") {
      demoScript = true;
    } else if (arg == "--presets" && i + 1 < argc) {
      presetDir = argv[++i];
    } else if (arg == "--sample-rate" && i + 1 < argc) {
      int value = std::stoi(argv[++i]);
      if (value <= 0) {
        std::fprintf(stderr, "--sample-rate must be positive\n");
        return 1;
      }
      requestedSampleRate = value;
    } else if (arg == "--channels" && i + 1 < argc) {
      int value = std::stoi(argv[++i]);
      if (value <= 0) {
        std::fprintf(stderr, "--channels must be positive\n");
        return 1;
      }
      requestedChannels = value;
    } else if (arg == "--help") {
      showHelp = true;
    } else {
      std::fprintf(stderr, "Unknown argument: %s\n", arg.c_str());
      printUsage();
      return 1;
    }
  }

  if (showHelp) {
    printUsage();
    return 0;
  }

  if (!headless && !wavPath.empty()) {
    std::fprintf(stderr, "--wav requires --headless\n");
    return 1;
  }

  if (headless) {
    if (wavPath.empty() || presetPath.empty() || frames <= 0) {
      std::fprintf(stderr, "--headless requires --wav, --preset and --frames\n");
      return 1;
    }
    bool writePngs = outPath != ".";
    return runHeadless(wavPath, presetPath, frames, outPath, writePngs);
  }

  avs::Window window(1920, 1080, "AVS Player");
  avs::AudioInputConfig audioConfig;
  if (requestedSampleRate) {
    audioConfig.requestedSampleRate = requestedSampleRate;
  }
  if (requestedChannels) {
    audioConfig.engineChannels = std::max(1, *requestedChannels);
    audioConfig.requestedChannels = requestedChannels;
  }
  avs::AudioInput audio(audioConfig);
  if (!audio.ok()) {
    std::fprintf(stderr, "audio init failed\n");
    return 1;
  }

  avs::Engine engine(1920, 1080);
  std::filesystem::path currentPreset;
  std::unique_ptr<avs::FileWatcher> watcher;
  auto loadPreset = [&]() -> bool {
    if (currentPreset.empty()) return false;
    auto parsed = avs::parsePreset(currentPreset);
    if (!parsed.warnings.empty()) {
      for (const auto& w : parsed.warnings) {
        std::fprintf(stderr, "%s\n", w.c_str());
      }
    }
    bool success = !parsed.chain.empty();
    if (!success) {
      std::fprintf(stderr, "failed to parse preset: %s\n", currentPreset.string().c_str());
    } else {
      engine.setChain(std::move(parsed.chain));
    }
    watcher = std::make_unique<avs::FileWatcher>(currentPreset);
    return success;
  };

  bool chainConfigured = false;
  if (!presetPath.empty()) {
    currentPreset = presetPath;
    chainConfigured = loadPreset();
  }

  if (!chainConfigured && !presetDir.empty()) {
    for (auto& e : std::filesystem::directory_iterator(presetDir)) {
      if (e.is_regular_file() && e.path().extension() == ".avs") {
        currentPreset = e.path();
        chainConfigured = loadPreset();
        if (chainConfigured) break;
      }
    }
  }

  std::vector<std::unique_ptr<avs::Effect>> chain;
  if (!chainConfigured && demoScript) {
    std::string frameScript;
    std::string pixelScript =
        "red = clamp(sin(x*0.01 + time)*bass,0,1);"
        "green = clamp(sin(y*0.01 + time)*mid,0,1);"
        "blue = clamp(sin((x+y)*0.01 + time)*treb,0,1);";
    chain.push_back(std::make_unique<avs::ScriptedEffect>(frameScript, pixelScript));
    engine.setChain(std::move(chain));
    chainConfigured = true;
  }

  if (!chainConfigured) {
    chain.push_back(std::make_unique<avs::BlurEffect>());
    chain.push_back(std::make_unique<avs::ColorMapEffect>());
    chain.push_back(std::make_unique<avs::ConvolutionEffect>());
    engine.setChain(std::move(chain));
    chainConfigured = true;
  }

  if (!currentPreset.empty() && !watcher) {
    watcher = std::make_unique<avs::FileWatcher>(currentPreset);
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
