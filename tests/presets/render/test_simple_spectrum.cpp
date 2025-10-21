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
#include <vector>

#include "audio/analyzer.h"
#include "avs/core/EffectRegistry.hpp"
#include "avs/core/Pipeline.hpp"
#include "avs/core/RenderContext.hpp"
#include "avs/effects/RegisterEffects.hpp"

namespace {

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

struct SyntheticAudio {
  int sampleRate = 44100;
  avs::audio::Analysis analysis{};

  const avs::audio::Analysis& step(int frameIndex) {
    constexpr double pi = 3.14159265358979323846;
    for (std::size_t i = 0; i < avs::audio::Analysis::kWaveformSize; ++i) {
      const double t = static_cast<double>(frameIndex * avs::audio::Analysis::kWaveformSize + i) /
                       static_cast<double>(sampleRate);
      double sample = 0.5 * std::sin(2.0 * pi * 90.0 * t);
      sample += 0.3 * std::sin(2.0 * pi * 420.0 * t);
      sample += 0.2 * std::sin(2.0 * pi * 1600.0 * t);
      if (frameIndex % 3 == 0) {
        const double pulse = std::exp(-4.0 * static_cast<double>(i) /
                                      static_cast<double>(avs::audio::Analysis::kWaveformSize));
        sample += 0.6 * pulse;
      }
      analysis.waveform[i] = static_cast<float>(std::clamp(sample, -1.0, 1.0));
    }

    const bool beatFrame = (frameIndex % 3) == 0;
    auto gaussian = [](double x, double mu, double sigma) {
      const double norm = (x - mu) / sigma;
      return std::exp(-norm * norm);
    };
    for (std::size_t i = 0; i < avs::audio::Analysis::kSpectrumSize; ++i) {
      const double freq = static_cast<double>(i) * static_cast<double>(sampleRate) /
                          static_cast<double>(avs::audio::Analysis::kFftSize);
      double magnitude = 0.55 * gaussian(freq, 90.0, 30.0);
      magnitude += 0.40 * gaussian(freq, 420.0, 80.0);
      magnitude += 0.30 * gaussian(freq, 1600.0, 250.0);
      if (beatFrame) {
        magnitude += 0.45 * gaussian(freq, 140.0, 70.0);
      }
      analysis.spectrum[i] = static_cast<float>(magnitude);
    }
    analysis.beat = beatFrame;
    analysis.bpm = beatFrame ? 100.0f : 0.0f;
    analysis.bass = beatFrame ? 0.85f : 0.55f;
    analysis.mid = 0.45f;
    analysis.treb = 0.35f;
    analysis.confidence = beatFrame ? 0.8f : 0.3f;
    return analysis;
  }
};

FrameResult renderSimpleSpectrum(const avs::core::ParamBlock& params) {
  avs::core::EffectRegistry registry;
  avs::effects::registerCoreEffects(registry);

  avs::core::Pipeline pipeline(registry);
  pipeline.add("Render / Simple", params);

  SyntheticAudio audio;

  FrameResult result;
  result.pixels.resize(static_cast<std::size_t>(kWidth) * static_cast<std::size_t>(kHeight) * 4u);

  avs::core::RenderContext context;
  context.width = kWidth;
  context.height = kHeight;
  context.framebuffer = {result.pixels.data(), result.pixels.size()};

  for (int frame = 0; frame < kFrames; ++frame) {
    const auto& analysis = audio.step(frame);
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

void expectGolden(const FrameResult& frame, const std::filesystem::path& goldenPath, bool update = false) {
  if (update) {
    std::ofstream out(goldenPath);
    ASSERT_TRUE(out.good());
    out << frame.hash << "\n";
    return;
  }
  const auto golden = loadGolden(goldenPath);
  if (golden.empty()) {
    ADD_FAILURE() << "Golden file missing or empty: " << goldenPath << " (hash " << frame.hash << ")";
    return;
  }
  EXPECT_EQ(frame.hash, golden.front());
}

}  // namespace

TEST(SimpleSpectrumRenderTest, GoldenSnapshot) {
  avs::core::ParamBlock params;
  params.setInt("effect", (2 << 2) | (2 << 4));
  params.setInt("num_colors", 3);
  params.setInt("color0", 0x00FF0000);
  params.setInt("color1", 0x0000FF00);
  params.setInt("color2", 0x000000FF);
  const auto golden =
      std::filesystem::path(SOURCE_DIR) / "tests/presets/render/golden/simple_spectrum.txt";
  const auto frame = renderSimpleSpectrum(params);
  expectGolden(frame, golden);
}

