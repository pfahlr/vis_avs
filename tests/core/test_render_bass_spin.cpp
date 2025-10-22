#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include <avs/audio/analyzer.h>
#include <avs/core/ParamBlock.hpp>
#include <avs/core/RenderContext.hpp>
#include <avs/effects/render/effect_bass_spin.h>

namespace {

std::string hashFNV1a(const std::vector<std::uint8_t>& data) {
  constexpr std::uint32_t kPrime = 16777619u;
  std::uint32_t hash = 2166136261u;
  for (std::uint8_t byte : data) {
    hash ^= byte;
    hash *= kPrime;
  }
  std::array<char, 9> buffer{};
  std::snprintf(buffer.data(), buffer.size(), "%08x", hash);
  return std::string(buffer.data());
}

avs::audio::Analysis makeAnalysis() {
  avs::audio::Analysis analysis{};
  for (std::size_t i = 0; i < analysis.waveform.size(); ++i) {
    const float t = static_cast<float>(i) / static_cast<float>(analysis.waveform.size() - 1);
    analysis.waveform[i] = std::sin(t * 6.2831853f) * 0.75f;
  }
  for (std::size_t i = 0; i < analysis.spectrum.size(); ++i) {
    const float t = static_cast<float>(i) / static_cast<float>(analysis.spectrum.size());
    analysis.spectrum[i] = (1.0f + std::cos(t * 3.1415926f)) * 0.5f;
  }
  analysis.bass = 0.5f;
  return analysis;
}

avs::core::RenderContext makeContext(int width, int height, std::vector<std::uint8_t>& pixels,
                                     avs::audio::Analysis& analysis) {
  avs::core::RenderContext context{};
  context.width = width;
  context.height = height;
  context.framebuffer = {pixels.data(), pixels.size()};
  context.audioAnalysis = &analysis;
  context.audioSpectrum = {analysis.spectrum.data(), analysis.spectrum.size()};
  context.deltaSeconds = 1.0 / 60.0;
  return context;
}

}  // namespace

TEST(RenderBassSpinTest, TrianglesHashStable) {
  avs::effects::render::BassSpin effect;
  avs::core::ParamBlock params;
  params.setString("mode", "triangles");
  params.setString("colors", "#FF4080,#40C0FF");
  params.setInt("enabled", 3);
  effect.setParams(params);

  auto analysis = makeAnalysis();
  std::vector<std::uint8_t> pixels(96u * 64u * 4u, 0u);
  auto context = makeContext(96, 64, pixels, analysis);

  for (int i = 0; i < 3; ++i) {
    context.frameIndex = static_cast<std::uint64_t>(i);
    context.rng.reseed(context.frameIndex);
    ASSERT_TRUE(effect.render(context));
  }

  EXPECT_EQ(hashFNV1a(pixels), "29030583");
}

TEST(RenderBassSpinTest, LineModeSingleChannelHash) {
  avs::effects::render::BassSpin effect;
  avs::core::ParamBlock params;
  params.setString("mode", "lines");
  params.setInt("enabled", 1);
  params.setInt("color0", 0x80FF40);
  params.setInt("color1", 0x2040FF);
  effect.setParams(params);

  auto analysis = makeAnalysis();
  std::vector<std::uint8_t> pixels(80u * 80u * 4u, 0u);
  auto context = makeContext(80, 80, pixels, analysis);

  for (int i = 0; i < 4; ++i) {
    context.frameIndex = static_cast<std::uint64_t>(i);
    context.rng.reseed(context.frameIndex);
    ASSERT_TRUE(effect.render(context));
  }

  EXPECT_EQ(hashFNV1a(pixels), "7f41c7ad");
}
