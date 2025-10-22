#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <avs/audio/analyzer.h>
#include <avs/core/EffectRegistry.hpp>
#include <avs/core/Pipeline.hpp>
#include <avs/core/RenderContext.hpp>
#include <avs/effects/RegisterEffects.hpp>

namespace {

using avs::audio::Analysis;
using avs::audio::Analyzer;

constexpr int kWidth = 160;
constexpr int kHeight = 120;
constexpr int kFrames = 10;

struct FrameResult {
  std::vector<std::uint8_t> pixels;
  std::string hash;
};

std::string hashFNV1a(const std::vector<std::uint8_t>& data) {
  constexpr std::uint64_t kOffset = 1469598103934665603ull;
  constexpr std::uint64_t kPrime = 1099511628211ull;
  std::uint64_t hash = kOffset;
  for (auto byte : data) {
    hash ^= byte;
    hash *= kPrime;
  }
  std::ostringstream oss;
  oss << std::hex << std::setfill('0') << std::setw(16) << hash;
  return oss.str();
}

struct AudioFixture {
  int sampleRate = 44100;
  int channels = 2;
  Analyzer analyzer{sampleRate, channels};
  std::vector<float> buffer;

  AudioFixture() { analyzer.setDampingEnabled(true); }

  const Analysis& step(int frameIndex) {
    const std::size_t frameSamples = Analysis::kFftSize;
    buffer.resize(frameSamples * static_cast<std::size_t>(channels));
    constexpr double pi = 3.14159265358979323846;
    for (std::size_t i = 0; i < frameSamples; ++i) {
      const double t = static_cast<double>(frameIndex * Analysis::kFftSize + i) /
                       static_cast<double>(sampleRate);
      double sample = 0.45 * std::sin(2.0 * pi * 60.0 * t);
      sample += 0.35 * std::sin(2.0 * pi * 440.0 * t);
      sample += 0.20 * std::sin(2.0 * pi * 2200.0 * t);
      if (frameIndex % 4 == 0) {
        const double decay = std::exp(-5.0 * static_cast<double>(i) / frameSamples);
        sample += 0.8 * decay;
      }
      const float clamped = static_cast<float>(std::clamp(sample, -1.0, 1.0));
      for (int ch = 0; ch < channels; ++ch) {
        buffer[i * static_cast<std::size_t>(channels) + static_cast<std::size_t>(ch)] = clamped;
      }
    }
    return analyzer.process(buffer.data(), frameSamples);
  }
};

FrameResult renderEffect(const std::string& effectKey, const avs::core::ParamBlock& params) {
  avs::core::EffectRegistry registry;
  avs::effects::registerCoreEffects(registry);

  avs::core::Pipeline pipeline(registry);
  pipeline.add(effectKey, params);

  AudioFixture audio;

  FrameResult result;
  result.pixels.resize(static_cast<std::size_t>(kWidth) * static_cast<std::size_t>(kHeight) * 4u);

  avs::core::RenderContext context;
  context.width = kWidth;
  context.height = kHeight;
  context.framebuffer = {result.pixels.data(), result.pixels.size()};

  for (int frame = 0; frame < kFrames; ++frame) {
    const Analysis& analysis = audio.step(frame);
    context.frameIndex = static_cast<std::uint64_t>(frame);
    context.deltaSeconds = 1.0 / 60.0;
    context.audioAnalysis = &analysis;
    context.audioSpectrum = {analysis.spectrum.data(), analysis.spectrum.size()};
    std::fill(result.pixels.begin(), result.pixels.end(), 0u);
    pipeline.render(context);
  }

  result.hash = hashFNV1a(result.pixels);
  return result;
}

std::vector<std::string> loadGolden(const std::filesystem::path& goldenPath) {
  std::vector<std::string> hashes;
  std::ifstream in(goldenPath);
  std::string line;
  while (std::getline(in, line)) {
    if (!line.empty()) {
      hashes.push_back(line);
    }
  }
  return hashes;
}

void expectGolden(const FrameResult& frame, const std::filesystem::path& goldenPath,
                  bool update = false) {
  if (update) {
    std::ofstream out(goldenPath);
    ASSERT_TRUE(out.good());
    out << frame.hash << "\n";
    return;
  }
  const auto golden = loadGolden(goldenPath);
  if (golden.empty()) {
    ADD_FAILURE() << "Golden file missing or empty: " << goldenPath << " (hash " << frame.hash
                  << ")";
    return;
  }
  EXPECT_EQ(frame.hash, golden.front());
}

}  // namespace

TEST(AudioOverlayGoldenTest, WaveGolden) {
  avs::core::ParamBlock params;
  params.setFloat("gain", 1.0f);
  params.setBool("damp", true);
  const auto golden = std::filesystem::path(SOURCE_DIR) / "tests/presets/audio_vis/golden/wave.txt";
  const auto frame = renderEffect("effect_wave", params);
  expectGolden(frame, golden);
}

TEST(AudioOverlayGoldenTest, SpectrumGolden) {
  avs::core::ParamBlock params;
  params.setFloat("gain", 0.6f);
  params.setBool("damp", true);
  const auto golden =
      std::filesystem::path(SOURCE_DIR) / "tests/presets/audio_vis/golden/spectrum.txt";
  const auto frame = renderEffect("effect_spec", params);
  expectGolden(frame, golden);
}

TEST(AudioOverlayGoldenTest, BandsGolden) {
  avs::core::ParamBlock params;
  params.setFloat("gain", 1.2f);
  params.setBool("damp", true);
  const auto golden =
      std::filesystem::path(SOURCE_DIR) / "tests/presets/audio_vis/golden/bands.txt";
  const auto frame = renderEffect("effect_bands", params);
  expectGolden(frame, golden);
}

TEST(AudioOverlayGoldenTest, LevelTextGolden) {
  avs::core::ParamBlock params;
  params.setBool("damp", false);
  const auto golden =
      std::filesystem::path(SOURCE_DIR) / "tests/presets/audio_vis/golden/leveltext.txt";
  const auto frame = renderEffect("effect_leveltext", params);
  expectGolden(frame, golden);
}

TEST(AudioOverlayGoldenTest, BandTextGolden) {
  avs::core::ParamBlock params;
  params.setFloat("gain", 1.1f);
  params.setBool("damp", false);
  const auto golden =
      std::filesystem::path(SOURCE_DIR) / "tests/presets/audio_vis/golden/bandtext.txt";
  const auto frame = renderEffect("effect_bandtxt", params);
  expectGolden(frame, golden);
}

TEST(AudioVisRenderEffectGolden, DotPlaneGolden) {
  avs::core::ParamBlock params;
  params.setInt("rotvel", 14);
  params.setInt("angle", -25);
  params.setInt("color2", 0x7A2F2F);
  params.setInt("color3", 0xD450A0);
  const auto golden =
      std::filesystem::path(SOURCE_DIR) / "tests/presets/audio_vis/golden/dot_plane.txt";
  const auto frame = renderEffect("render / dot plane", params);
  expectGolden(frame, golden);
}
