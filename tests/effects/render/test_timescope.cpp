#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <vector>

#include "audio/analyzer.h"
#include "avs/core/ParamBlock.hpp"
#include "avs/core/RenderContext.hpp"
#include "effects/render/effect_timescope.h"

namespace {

avs::core::RenderContext makeContext(std::vector<std::uint8_t>& pixels, int width, int height,
                                     const avs::audio::Analysis& analysis) {
  avs::core::RenderContext ctx{};
  ctx.width = width;
  ctx.height = height;
  ctx.framebuffer = {pixels.data(), pixels.size()};
  ctx.audioAnalysis = &analysis;
  ctx.audioSpectrum = {nullptr, 0};
  return ctx;
}

}  // namespace

TEST(RenderTimescope, RendersAmplitudeColumn) {
  constexpr int kWidth = 4;
  constexpr int kHeight = 8;
  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(kWidth * kHeight * 4u), 0);

  avs::audio::Analysis analysis{};
  analysis.waveform.fill(1.0f);

  avs::effects::render::Timescope effect;
  avs::core::ParamBlock params;
  params.setInt("color", 0xFF0000);
  params.setInt("nbands", 32);
  params.setFloat("thickness", 0.1f);
  effect.setParams(params);

  auto ctx = makeContext(pixels, kWidth, kHeight, analysis);
  ASSERT_TRUE(effect.render(ctx));

  const auto pixelAt = [&](int x, int y) {
    const std::size_t index = static_cast<std::size_t>(y * kWidth + x) * 4u;
    return std::array<std::uint8_t, 4>{pixels[index + 0], pixels[index + 1], pixels[index + 2],
                                       pixels[index + 3]};
  };

  // With a constant waveform of 1.0, the top rows should be saturated red and the bottom rows remain dark.
  auto topPixel = pixelAt(0, 0);
  auto bottomPixel = pixelAt(0, kHeight - 1);
  EXPECT_GT(topPixel[0], 200);
  EXPECT_EQ(topPixel[1], 0);
  EXPECT_EQ(topPixel[2], 0);
  EXPECT_EQ(topPixel[3], topPixel[0]);
  EXPECT_EQ(bottomPixel[0], 0);
  EXPECT_EQ(bottomPixel[1], 0);
  EXPECT_EQ(bottomPixel[2], 0);
  EXPECT_EQ(bottomPixel[3], 0);
}

TEST(RenderTimescope, ClearsColumnWhenContributionZero) {
  constexpr int kWidth = 2;
  constexpr int kHeight = 4;
  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(kWidth * kHeight * 4u), 255);

  avs::audio::Analysis analysis{};
  analysis.waveform.fill(0.0f);

  avs::effects::render::Timescope effect;
  avs::core::ParamBlock params;
  params.setInt("color", 0x00FF00);
  params.setInt("blend", 0);
  effect.setParams(params);

  auto ctx = makeContext(pixels, kWidth, kHeight, analysis);
  ASSERT_TRUE(effect.render(ctx));

  for (int y = 0; y < kHeight; ++y) {
    const auto idx = static_cast<std::size_t>(y * kWidth) * 4u;
    EXPECT_EQ(pixels[idx + 0], 0);
    EXPECT_EQ(pixels[idx + 1], 0);
    EXPECT_EQ(pixels[idx + 2], 0);
    EXPECT_EQ(pixels[idx + 3], 0);
  }
}

