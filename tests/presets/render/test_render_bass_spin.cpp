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

#include "audio/analyzer.h"
#include "avs/core/EffectRegistry.hpp"
#include "avs/core/Pipeline.hpp"
#include "avs/core/RenderContext.hpp"
#include "avs/effects/RegisterEffects.hpp"

namespace {

constexpr int kWidth = 160;
constexpr int kHeight = 120;
constexpr int kFrames = 12;

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

avs::audio::Analysis makeAnalysis(int frame) {
  avs::audio::Analysis analysis{};
  for (std::size_t i = 0; i < avs::audio::Analysis::kSpectrumSize; ++i) {
    const double t = static_cast<double>(frame) / static_cast<double>(kFrames);
    const double bassPulse = 0.7 + 0.3 * std::sin((t + 0.05 * i) * 2.0 * 3.14159265358979323846);
    const double falloff = std::exp(-static_cast<double>(i) / 96.0);
    analysis.spectrum[i] = static_cast<float>(bassPulse * falloff);
  }
  analysis.beat = (frame % 4) == 0;
  analysis.bass = analysis.spectrum[0];
  return analysis;
}

FrameResult renderBassSpin(const avs::core::ParamBlock& params) {
  avs::core::EffectRegistry registry;
  avs::effects::registerCoreEffects(registry);

  avs::core::Pipeline pipeline(registry);
  pipeline.add("Render / Bass Spin", params);

  FrameResult result;
  result.pixels.resize(static_cast<std::size_t>(kWidth) * static_cast<std::size_t>(kHeight) * 4u);

  avs::core::RenderContext context;
  context.width = kWidth;
  context.height = kHeight;
  context.framebuffer = {result.pixels.data(), result.pixels.size()};

  for (int frame = 0; frame < kFrames; ++frame) {
    const auto analysis = makeAnalysis(frame);
    context.frameIndex = static_cast<std::uint64_t>(frame);
    context.deltaSeconds = 1.0 / 60.0;
    context.audioAnalysis = &analysis;
    context.audioSpectrum = {analysis.spectrum.data(), analysis.spectrum.size()};
    std::fill(result.pixels.begin(), result.pixels.end(), 0u);
    pipeline.render(context);
    context.audioAnalysis = nullptr;
    context.audioSpectrum = {nullptr, 0};
  }

  result.hash = hashFNV1a(result.pixels);
  return result;
}

std::vector<std::string> loadGolden(const std::filesystem::path& path) {
  std::vector<std::string> hashes;
  std::ifstream in(path);
  if (!in.is_open()) {
    return hashes;
  }
  std::string line;
  while (std::getline(in, line)) {
    if (!line.empty()) {
      hashes.push_back(line);
    }
  }
  return hashes;
}

void expectGolden(const std::string& name, const FrameResult& frame) {
  const auto goldenPath = std::filesystem::path(SOURCE_DIR) / "tests" / "presets" / "render" /
                          "golden" / (name + ".txt");
  const auto golden = loadGolden(goldenPath);
  if (golden.empty()) {
    ADD_FAILURE() << "Missing golden hashes for " << name << " at " << goldenPath
                  << "\nCaptured hash: " << frame.hash;
    return;
  }
  ASSERT_EQ(golden.size(), 1u);
  EXPECT_EQ(golden.front(), frame.hash) << "Golden mismatch for " << name;
}

TEST(RenderBassSpin, TrianglesGolden) {
  avs::core::ParamBlock params;
  params.setInt("enabled", 3);
  params.setInt("mode", 1);
  params.setString("color_left", "#ff6a00");
  params.setString("color_right", "#00c8ff");
  const auto frame = renderBassSpin(params);
  expectGolden("bass_spin_triangles", frame);
}

TEST(RenderBassSpin, LinesWithSingleArmGolden) {
  avs::core::ParamBlock params;
  params.setInt("mode", 0);
  params.setBool("right_enabled", false);
  params.setString("color_left", "#ffffff");
  const auto frame = renderBassSpin(params);
  expectGolden("bass_spin_lines", frame);
}

}  // namespace
