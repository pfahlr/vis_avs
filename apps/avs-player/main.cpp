#include <algorithm>
#include <array>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <mutex>
#include <memory>
#include <optional>
#include <variant>
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

#include <avs/audio.hpp>
#include <avs/audio/AudioEngine.hpp>
#include <avs/audio/DeviceInfo.hpp>
#include <avs/effects.hpp>
#include <avs/engine.hpp>
#include <avs/fft.hpp>
#include <avs/fs.hpp>
#include <avs/preset.hpp>
#include <avs/runtime/ResourceManager.hpp>
#include <avs/window.hpp>

namespace {

avs::runtime::ResourceManager& resourceManager() {
  static avs::runtime::ResourceManager manager;
  return manager;
}

void logResourceSearchPaths() {
  const auto paths = resourceManager().search_paths();
  std::printf("Resource search paths:\n");
  for (const auto& path : paths) {
    std::printf("  %s\n", path.string().c_str());
  }
}

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
      "                 [--sample-rate <hz|default>] [--channels <count|default>] [--input-device "
      "<id>]\n"
      "                 [--list-input-devices] [--demo-script] [--presets <directory>] [--help]\n");
}

void printInputDevices(const std::vector<avs::audio::DeviceInfo>& devices) {
  if (devices.empty()) {
    std::printf("No audio capture devices detected.\n");
    return;
  }

  std::printf("%-6s %-40s %-7s %-7s %-10s %-10s\n", "Index", "Name", "Inputs", "Outputs",
              "Default", "Rate(Hz)");
  for (const auto& device : devices) {
    std::string defaults;
    if (device.isDefaultInput) defaults.push_back('I');
    if (device.isDefaultOutput) defaults.push_back('O');
    if (defaults.empty()) defaults = "-";
    std::printf("%-6d %-40.40s %-7d %-7d %-10s %-10.0f\n", device.index, device.name.c_str(),
                device.maxInputChannels, device.maxOutputChannels, defaults.c_str(),
                device.defaultSampleRate);
  }
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

    avs::AudioState::LegacyBuffer specLegacy{};
    specLegacy.fill(0.0f);
    const size_t legacyCount = avs::AudioState::kLegacyVisSamples;
    const size_t spectrumSize = spectrum_.size();
    if (legacyCount > 0 && spectrumSize > 0) {
      const double scale = static_cast<double>(spectrumSize) / static_cast<double>(legacyCount);
      for (size_t i = 0; i < legacyCount; ++i) {
        size_t begin = static_cast<size_t>(std::floor(static_cast<double>(i) * scale));
        size_t end = static_cast<size_t>(std::floor(static_cast<double>(i + 1) * scale));
        if (end <= begin) end = std::min(begin + 1, spectrumSize);
        double sum = 0.0;
        size_t count = 0;
        for (size_t j = begin; j < end && j < spectrumSize; ++j) {
          sum += spectrum_[j];
          ++count;
        }
        float value = 0.0f;
        if (count > 0) {
          value = static_cast<float>(sum / static_cast<double>(count));
        } else if (begin < spectrumSize) {
          value = spectrum_[begin];
        }
        specLegacy[i] = value;
      }
    }
    state.spectrumLegacy[0] = specLegacy;
    state.spectrumLegacy[1] = specLegacy;

    for (auto& scope : state.oscilloscope) {
      scope.fill(0.0f);
    }
    const size_t legacySamples = avs::AudioState::kLegacyVisSamples;
    const size_t channelCount = static_cast<size_t>(wav_.channels);
    if (legacySamples > 0 && channelCount > 0) {
      const size_t sampleStart =
          kFftSize > legacySamples ? static_cast<size_t>(kFftSize) - legacySamples : 0;
      for (unsigned int ch = 0; ch < std::min<unsigned int>(wav_.channels, 2); ++ch) {
        auto& dest = state.oscilloscope[static_cast<size_t>(ch)];
        for (size_t i = 0; i < legacySamples; ++i) {
          size_t sampleIndex = sampleStart + i;
          if (sampleIndex >= static_cast<size_t>(kFftSize)) break;
          long idx = start + static_cast<long>(sampleIndex) * static_cast<long>(wav_.channels) +
                     static_cast<long>(ch);
          float sample = 0.0f;
          if (idx >= 0 && idx < static_cast<long>(wav_.samples.size())) {
            sample = wav_.samples[static_cast<size_t>(idx)];
          }
          dest[i] = sample;
        }
      }
      if (wav_.channels == 1) {
        state.oscilloscope[1] = state.oscilloscope[0];
      }
    }

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

class LiveAudioAnalyzer {
 public:
  explicit LiveAudioAnalyzer(int sampleRate)
      : sampleRate_(sampleRate),
        fft_(kFftSize),
        mono_(kFftSize, 0.0f),
        leftWindow_(kFftSize, 0.0f),
        rightWindow_(kFftSize, 0.0f),
        spectrum_(kFftSize / 2, 0.0f) {
    ringLeft_.assign(kRingSize, 0.0f);
    ringRight_.assign(kRingSize, 0.0f);
  }

  void setSampleRate(int sampleRate) {
    std::lock_guard<std::mutex> lock(mutex_);
    sampleRate_ = sampleRate;
  }

  void setOutputChannelCount(int channels) {
    std::lock_guard<std::mutex> lock(mutex_);
    requestedChannels_ = channels > 0 ? channels : 1;
  }

  void pushSamples(const float* samples, unsigned long frames, int channels, double streamTime) {
    if (frames == 0) return;
    if (channels <= 0) channels = 1;

    std::lock_guard<std::mutex> lock(mutex_);
    const size_t frameCount = static_cast<size_t>(frames);
    const size_t channelCount = static_cast<size_t>(channels);
    const size_t mask = kRingSize - 1;

    zeroBuffer_.resize(frameCount * channelCount);
    const float* data = samples;
    if (!data) {
      std::fill(zeroBuffer_.begin(), zeroBuffer_.end(), 0.0f);
      data = zeroBuffer_.data();
    }

    for (size_t frame = 0; frame < frameCount; ++frame) {
      size_t base = frame * channelCount;
      float left = data[base];
      float right = channels > 1 ? data[base + 1] : left;
      if (channels > 2) {
        float sum = left + right;
        for (int ch = 2; ch < channels; ++ch) {
          sum += data[base + static_cast<size_t>(ch)];
        }
        float avg = sum / static_cast<float>(channels);
        left = avg;
        right = avg;
      }
      ringLeft_[(writeIndex_ + frame) & mask] = left;
      ringRight_[(writeIndex_ + frame) & mask] = right;
    }

    writeIndex_ = (writeIndex_ + frameCount) & mask;
    framesFilled_ = std::min<size_t>(framesFilled_ + frameCount, kRingSize);
    channelCount_ = channels;
    lastStreamTime_ = streamTime;
  }

  avs::AudioState poll() {
    avs::AudioState state;
    const size_t mask = kRingSize - 1;

    int channels = 1;
    double streamTime = 0.0;
    int sampleRate = 0;
    int requestedChannels = 0;

    {
      std::lock_guard<std::mutex> lock(mutex_);
      channels = std::max(1, channelCount_);
      streamTime = lastStreamTime_;
      sampleRate = sampleRate_;
      requestedChannels = requestedChannels_;
      std::fill(leftWindow_.begin(), leftWindow_.end(), 0.0f);
      std::fill(rightWindow_.begin(), rightWindow_.end(), 0.0f);

      size_t available = std::min(framesFilled_, kRingSize);
      size_t framesToCopy = std::min<size_t>(available, kFftSize);
      size_t start = (writeIndex_ + kRingSize - framesToCopy) & mask;
      for (size_t i = 0; i < framesToCopy; ++i) {
        size_t ringIndex = (start + i) & mask;
        size_t dest = kFftSize - framesToCopy + i;
        leftWindow_[dest] = ringLeft_[ringIndex];
        rightWindow_[dest] = ringRight_[ringIndex];
      }
    }

    for (size_t i = 0; i < kFftSize; ++i) {
      float left = leftWindow_[i];
      float right = channels > 1 ? rightWindow_[i] : leftWindow_[i];
      if (channels == 1) {
        right = left;
      }
      leftWindow_[i] = left;
      rightWindow_[i] = right;
      mono_[i] = 0.5f * (left + right);
    }

    float sumSq = 0.0f;
    for (float v : mono_) sumSq += v * v;
    if (kFftSize > 0) {
      state.rms = std::sqrt(sumSq / static_cast<float>(kFftSize));
    } else {
      state.rms = 0.0f;
    }

    fft_.compute(mono_.data(), spectrum_);
    state.spectrum = spectrum_;

    avs::AudioState::LegacyBuffer legacy{};
    legacy.fill(0.0f);
    const size_t legacyCount = avs::AudioState::kLegacyVisSamples;
    if (!spectrum_.empty()) {
      double scale = static_cast<double>(spectrum_.size()) / static_cast<double>(legacyCount);
      for (size_t i = 0; i < legacyCount; ++i) {
        size_t begin = static_cast<size_t>(std::floor(static_cast<double>(i) * scale));
        size_t end = static_cast<size_t>(std::floor(static_cast<double>(i + 1) * scale));
        if (end <= begin) {
          end = begin + 1;
        }
        double sum = 0.0;
        size_t count = 0;
        for (size_t j = begin; j < end && j < spectrum_.size(); ++j) {
          sum += spectrum_[j];
          ++count;
        }
        float value = 0.0f;
        if (count > 0) {
          value = static_cast<float>(sum / static_cast<double>(count));
        } else if (begin < spectrum_.size()) {
          value = spectrum_[begin];
        }
        legacy[i] = value;
      }
    }
    state.spectrumLegacy[0] = legacy;
    state.spectrumLegacy[1] = legacy;

    state.oscilloscope[0].fill(0.0f);
    state.oscilloscope[1].fill(0.0f);
    size_t oscCount = avs::AudioState::kLegacyVisSamples;
    size_t oscStart = kFftSize > oscCount ? kFftSize - oscCount : 0;
    for (size_t i = 0; i < oscCount && (oscStart + i) < kFftSize; ++i) {
      state.oscilloscope[0][i] = leftWindow_[oscStart + i];
      state.oscilloscope[1][i] = rightWindow_[oscStart + i];
    }

    std::array<float, 3> newBands{0.f, 0.f, 0.f};
    std::array<int, 3> counts{0, 0, 0};
    double binHz = sampleRate > 0 ? static_cast<double>(sampleRate) / static_cast<double>(kFftSize) : 0.0;
    for (size_t i = 0; i < spectrum_.size(); ++i) {
      double freq = binHz * static_cast<double>(i);
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
      if (counts[i] > 0) {
        newBands[i] /= static_cast<float>(counts[i]);
      }
      bands_[i] = bands_[i] * (1.0f - kBandSmooth) + newBands[i] * kBandSmooth;
    }
    state.bands = bands_;

    int reportedChannels = requestedChannels > 0 ? requestedChannels : channels;
    if (reportedChannels <= 1) {
      state.oscilloscope[1] = state.oscilloscope[0];
      state.spectrumLegacy[1] = state.spectrumLegacy[0];
    }

    state.timeSeconds = streamTime;
    state.sampleRate = sampleRate;
    state.inputSampleRate = sampleRate;
    state.channels = reportedChannels;

    return state;
  }

 private:
  static constexpr size_t kFftSize = 2048;
  static constexpr size_t kRingSize = 1 << 15;
  static constexpr float kBandSmooth = 0.2f;

  std::vector<float> ringLeft_;
  std::vector<float> ringRight_;
  size_t writeIndex_ = 0;
  size_t framesFilled_ = 0;
  int channelCount_ = 0;
  double lastStreamTime_ = 0.0;
  int sampleRate_ = 0;
  std::vector<float> zeroBuffer_;
  int requestedChannels_ = 0;

  avs::FFT fft_;
  std::vector<float> mono_;
  std::vector<float> leftWindow_;
  std::vector<float> rightWindow_;
  std::vector<float> spectrum_;
  std::array<float, 3> bands_{{0.f, 0.f, 0.f}};

  std::mutex mutex_;
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
  logResourceSearchPaths();
  bool headless = false;
  std::filesystem::path wavPath;
  std::filesystem::path presetPath;
  std::filesystem::path outPath = ".";
  int frames = 0;
  bool demoScript = false;
  std::filesystem::path presetDir;
  bool showHelp = false;
  int requestedSampleRate = 0;
  bool hasRequestedSampleRate = false;
  int requestedChannels = 0;
  bool hasRequestedChannels = false;
  std::optional<std::string> requestedInputDevice;
  bool listInputDevices = false;
  bool useDeviceDefaultSampleRate = true;

  std::unique_ptr<avs::audio::AudioEngine> audioEngine;
  std::vector<avs::audio::DeviceInfo> availableDevices;
  auto ensureAudioEngine = [&]() -> bool {
    if (audioEngine) {
      return true;
    }
    try {
      audioEngine = std::make_unique<avs::audio::AudioEngine>();
      availableDevices = audioEngine->listInputDevices();
      return true;
    } catch (const std::exception& ex) {
      std::fprintf(stderr, "%s\n", ex.what());
      return false;
    }
  };

  auto normalizeToken = [](std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
  };

  auto parsePositiveInt = [](const std::string& text) -> std::optional<int> {
    errno = 0;
    char* end = nullptr;
    long parsed = std::strtol(text.c_str(), &end, 10);
    if (end == text.c_str() || *end != '\0' || errno == ERANGE || parsed <= 0 ||
        parsed > std::numeric_limits<int>::max()) {
      return std::nullopt;
    }
    return static_cast<int>(parsed);
  };

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
      std::string token = argv[++i];
      std::string lowered = normalizeToken(token);
      if (lowered == "default" || lowered == "device-default" || lowered == "auto") {
        hasRequestedSampleRate = false;
        requestedSampleRate = 0;
        useDeviceDefaultSampleRate = true;
      } else {
        auto parsed = parsePositiveInt(token);
        if (!parsed.has_value()) {
          std::fprintf(stderr, "--sample-rate expects a positive integer or 'default'\n");
          return 1;
        }
        requestedSampleRate = parsed.value();
        hasRequestedSampleRate = true;
        useDeviceDefaultSampleRate = false;
      }
    } else if (arg == "--channels" && i + 1 < argc) {
      std::string token = argv[++i];
      std::string lowered = normalizeToken(token);
      if (lowered == "default" || lowered == "device-default" || lowered == "auto") {
        hasRequestedChannels = false;
        requestedChannels = 0;
      } else {
        auto parsed = parsePositiveInt(token);
        if (!parsed.has_value()) {
          std::fprintf(stderr, "--channels expects a positive integer or 'default'\n");
          return 1;
        }
        requestedChannels = parsed.value();
        hasRequestedChannels = true;
      }
    } else if (arg == "--input-device" && i + 1 < argc) {
      requestedInputDevice = argv[++i];
    } else if (arg == "--input-device") {
      std::fprintf(stderr, "--input-device requires an identifier\n");
      return 1;
    } else if (arg == "--list-input-devices") {
      listInputDevices = true;
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

  if (listInputDevices) {
    if (!ensureAudioEngine()) {
      return 1;
    }
    printInputDevices(availableDevices);
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

  if (!ensureAudioEngine()) {
    return 1;
  }
  if (availableDevices.empty()) {
    std::fprintf(stderr, "No audio capture devices are available.\n");
    return 1;
  }

  double selectionRate = 48000.0;
  if (hasRequestedSampleRate) {
    selectionRate = static_cast<double>(requestedSampleRate);
  }
  std::optional<avs::audio::DeviceSpecifier> deviceRequest;
  if (requestedInputDevice) {
    const std::string& token = *requestedInputDevice;
    char* end = nullptr;
    long parsed = std::strtol(token.c_str(), &end, 10);
    if (end != token.c_str() && *end == '\0') {
      if (parsed < std::numeric_limits<int>::min() || parsed > std::numeric_limits<int>::max()) {
        std::fprintf(stderr, "Input device index %s is out of range.\n", token.c_str());
        return 1;
      }
      deviceRequest = avs::audio::DeviceSpecifier{static_cast<int>(parsed)};
    } else {
      deviceRequest = avs::audio::DeviceSpecifier{token};
    }
  }

  avs::audio::DeviceInfo selectedDevice;
  try {
    selectedDevice = avs::audio::selectInputDevice(availableDevices, deviceRequest, selectionRate);
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "%s\n", ex.what());
    return 1;
  }

  double captureSampleRate = selectionRate;
  if (useDeviceDefaultSampleRate || !hasRequestedSampleRate) {
    if (selectedDevice.defaultSampleRate > 0.0) {
      captureSampleRate = selectedDevice.defaultSampleRate;
    }
  }
  if (captureSampleRate <= 0.0) {
    captureSampleRate = 48000.0;
  }

  LiveAudioAnalyzer analyzer(static_cast<int>(std::lround(captureSampleRate)));
  analyzer.setSampleRate(static_cast<int>(std::lround(captureSampleRate)));
  if (hasRequestedChannels) {
    int channelCount = requestedChannels;
    if (channelCount <= 0) {
      channelCount = 1;
    }
    analyzer.setOutputChannelCount(channelCount);
  }

  avs::audio::AudioEngine::InputStream inputStream;
  try {
    inputStream = audioEngine->openInputStream(
        selectedDevice, captureSampleRate, 0,
        [&analyzer](const float* data, unsigned long frames, int channels, double streamTime) {
          analyzer.pushSamples(data, frames, channels, streamTime);
        });
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "%s\n", ex.what());
    return 1;
  }

  std::printf("Capturing from device %d: %s (%.0f Hz)\n", selectedDevice.index,
              selectedDevice.name.c_str(), captureSampleRate);

  avs::Window window(1920, 1080, "AVS Player");

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
    if (parsed.chain.empty()) {
      std::fprintf(stderr, "failed to parse preset: %s\n", currentPreset.string().c_str());
      return false;
    }
    engine.setChain(std::move(parsed.chain));
    watcher = std::make_unique<avs::FileWatcher>(currentPreset);
    return true;
  };

  bool chainConfigured = false;
  if (!presetPath.empty()) {
    currentPreset = presetPath;
    chainConfigured = loadPreset();
    if (!chainConfigured) {
      std::fprintf(stderr, "Failed to load preset specified via --preset.\n");
      return 1;
    }
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

    auto s = analyzer.poll();
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
