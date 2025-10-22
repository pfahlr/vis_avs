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
#include "effects/render/effect_ring.h"

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
  avs::audio::Analysis analysis;
  for (std::size_t i = 0; i < analysis.waveform.size(); ++i) {
    const float t = static_cast<float>(i) / static_cast<float>(analysis.waveform.size() - 1);
    analysis.waveform[i] = std::sin(t * 3.1415926f) * 0.75f;
  }
  for (std::size_t i = 0; i < analysis.spectrum.size(); ++i) {
    const float t = static_cast<float>(i) / static_cast<float>(analysis.spectrum.size());
    analysis.spectrum[i] = (1.0f + std::cos(t * 6.2831853f)) * 0.5f;
  }
  analysis.beat = false;
  analysis.bass = 0.25f;
  analysis.mid = 0.5f;
  analysis.treb = 0.75f;
  return analysis;
}

avs::core::RenderContext makeContext(int width, int height, std::vector<std::uint8_t>& pixels,
                                     avs::audio::Analysis& analysis) {
  avs::core::RenderContext context;
  context.width = width;
  context.height = height;
  context.frameIndex = 0;
  context.deltaSeconds = 1.0 / 60.0;
  context.framebuffer = {pixels.data(), pixels.size()};
  context.audioAnalysis = &analysis;
  context.audioSpectrum = {analysis.spectrum.data(), analysis.spectrum.size()};
  return context;
}

}  // namespace

TEST(RenderRingTest, OscillatorHashStable) {
  avs::effects::render::Ring effect;
  avs::core::ParamBlock params;
  params.setString("colors", "#FF0000,#00FF00,#0000FF");
  params.setInt("size", 20);
  params.setString("source", "osc");
  params.setString("channel", "mix");
  params.setString("placement", "center");
  effect.setParams(params);

  auto analysis = makeAnalysis();
  std::vector<std::uint8_t> pixels(96u * 64u * 4u, 0u);
  auto context = makeContext(96, 64, pixels, analysis);

  ASSERT_TRUE(effect.render(context));
  const std::string hash = hashFNV1a(pixels);
  EXPECT_EQ(hash, "d8f04d66");
}

TEST(RenderRingTest, SpectrumEffectBitParsing) {
  avs::effects::render::Ring effect;
  avs::core::ParamBlock params;
  params.setInt("effect", (0 << 2) | (0 << 4));
  params.setInt("size", 24);
  params.setString("source", "spec");
  params.setString("colors", "#8080FF,#FF80FF");
  effect.setParams(params);

  auto analysis = makeAnalysis();
  for (std::size_t i = 0; i < analysis.waveform.size(); ++i) {
    analysis.waveform[i] = 0.0f;
  }
  std::vector<std::uint8_t> pixels(80u * 80u * 4u, 0u);
  auto context = makeContext(80, 80, pixels, analysis);

  ASSERT_TRUE(effect.render(context));
  const std::string hash = hashFNV1a(pixels);
  EXPECT_EQ(hash, "a39510b5");
}
