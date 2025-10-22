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
#include <avs/effects/legacy/render/effect_moving_particle.h>

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
    const float t = static_cast<float>(i) / static_cast<float>(analysis.waveform.size());
    analysis.waveform[i] = std::cos(t * 3.1415926f) * 0.25f;
  }
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

TEST(RenderMovingParticleTest, MotionAndBeatHashStable) {
  avs::effects::render::MovingParticle effect;
  avs::core::ParamBlock params;
  params.setInt("color", 0x40FF60);
  params.setInt("maxdist", 20);
  params.setInt("size", 6);
  params.setInt("size2", 12);
  params.setInt("blend", 1);
  params.setInt("enabled", 3);
  effect.setParams(params);

  auto analysis = makeAnalysis();
  std::vector<std::uint8_t> pixels(96u * 96u * 4u, 0u);
  auto context = makeContext(96, 96, pixels, analysis);

  for (int i = 0; i < 6; ++i) {
    context.frameIndex = static_cast<std::uint64_t>(i);
    context.rng.reseed(context.frameIndex);
    context.audioBeat = (i == 2 || i == 4);
    ASSERT_TRUE(effect.render(context));
    context.audioBeat = false;
  }

  EXPECT_EQ(hashFNV1a(pixels), "237d9185");
}
