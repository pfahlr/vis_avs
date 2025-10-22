#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "audio/analyzer.h"
#include "avs/core/ParamBlock.hpp"
#include "avs/core/RenderContext.hpp"
#include "effects/render/effect_timescope.h"

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
    analysis.waveform[i] = std::sin(t * 6.2831853f);
  }
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
  return context;
}

}  // namespace

TEST(RenderTimescopeTest, AlphaBlendProducesStableHash) {
  avs::effects::render::Timescope effect;
  avs::core::ParamBlock params;
  params.setInt("color", 0x40FF80);
  params.setString("blend_mode", "alpha");
  effect.setParams(params);

  auto analysis = makeAnalysis();
  std::vector<std::uint8_t> pixels(96u * 64u * 4u, 0u);
  auto context = makeContext(96, 64, pixels, analysis);

  for (int i = 0; i < context.width; ++i) {
    ASSERT_TRUE(effect.render(context));
  }

  const std::string hash = hashFNV1a(pixels);
  EXPECT_EQ(hash, "7f5cc205");
}

TEST(RenderTimescopeTest, AdditiveBlendAccumulatesEnergy) {
  avs::effects::render::Timescope effect;
  avs::core::ParamBlock params;
  params.setInt("color", 0xFF3366);
  params.setString("blend_mode", "add");
  params.setInt("bands", 128);
  effect.setParams(params);

  auto analysis = makeAnalysis();
  std::vector<std::uint8_t> pixels(80u * 48u * 4u, 0u);
  auto context = makeContext(80, 48, pixels, analysis);

  for (int i = 0; i < context.width / 2; ++i) {
    ASSERT_TRUE(effect.render(context));
  }

  const std::string hash = hashFNV1a(pixels);
  EXPECT_EQ(hash, "f3af3865");
}
