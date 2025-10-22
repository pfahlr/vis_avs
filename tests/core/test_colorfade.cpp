#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>

#include <avs/core/ParamBlock.hpp>
#include <avs/core/RenderContext.hpp>
#include <avs/core/DeterministicRng.hpp>
#include <avs/effects/trans/effect_colorfade.h>

namespace {

using avs::effects::trans::Colorfade;

avs::core::RenderContext makeContext(std::array<std::uint8_t, 16>& pixels) {
  avs::core::RenderContext context{};
  context.width = 4;
  context.height = 1;
  context.framebuffer = {pixels.data(), pixels.size()};
  return context;
}

}  // namespace

TEST(ColorfadeEffect, AppliesOffsetsBasedOnDominantChannel) {
  Colorfade effect;
  avs::core::ParamBlock params;
  params.setInt("offset_a", 10);
  params.setInt("offset_b", -5);
  params.setInt("offset_c", 3);
  effect.setParams(params);

  std::array<std::uint8_t, 16> pixels{
      250u, 20u, 10u, 255u,   // red dominant
      15u, 200u, 10u, 255u,   // green dominant
      5u, 40u, 230u, 255u,    // blue dominant
      120u, 120u, 120u, 255u  // neutral
  };

  avs::core::RenderContext context = makeContext(pixels);

  ASSERT_TRUE(effect.render(context));

  EXPECT_EQ(pixels[0], 245u);  // red reduced
  EXPECT_EQ(pixels[1], 30u);   // green boosted
  EXPECT_EQ(pixels[2], 13u);   // blue offset

  EXPECT_EQ(pixels[4], 18u);   // red after green dominant
  EXPECT_EQ(pixels[5], 195u);  // green reduced
  EXPECT_EQ(pixels[6], 20u);   // blue boosted

  EXPECT_EQ(pixels[8], 15u);   // red boosted for blue dominant
  EXPECT_EQ(pixels[9], 43u);   // green boosted
  EXPECT_EQ(pixels[10], 225u); // blue reduced

  EXPECT_EQ(pixels[12], 123u);
  EXPECT_EQ(pixels[13], 123u);
  EXPECT_EQ(pixels[14], 123u);
  EXPECT_EQ(pixels[15], 255u);
}

TEST(ColorfadeEffect, BeatOffsetsApplyWhenSmoothingEnabled) {
  Colorfade effect;
  avs::core::ParamBlock params;
  params.setBool("smooth", true);
  params.setInt("offset_a", 0);
  params.setInt("offset_b", 0);
  params.setInt("offset_c", 0);
  params.setInt("beat_offset_a", 5);
  params.setInt("beat_offset_b", -7);
  params.setInt("beat_offset_c", 11);
  effect.setParams(params);

  std::array<std::uint8_t, 8> pixel{200u, 120u, 90u, 255u, 0u, 0u, 0u, 0u};
  avs::core::RenderContext context{};
  context.width = 2;
  context.height = 1;
  context.framebuffer = {pixel.data(), pixel.size()};
  context.audioBeat = true;

  ASSERT_TRUE(effect.render(context));

  EXPECT_EQ(pixel[0], 193u);
  EXPECT_EQ(pixel[1], 125u);
  EXPECT_EQ(pixel[2], 101u);
}

TEST(ColorfadeEffect, RandomizesOffsetsDeterministicallyOnBeat) {
  Colorfade effect;
  avs::core::ParamBlock params;
  params.setBool("smooth", true);
  params.setBool("randomize", true);
  params.setInt("offset_a", 0);
  params.setInt("offset_b", 0);
  params.setInt("offset_c", 0);
  effect.setParams(params);

  std::array<std::uint8_t, 4> pixel{60u, 20u, 10u, 255u};
  avs::core::RenderContext context{};
  context.width = 1;
  context.height = 1;
  context.framebuffer = {pixel.data(), pixel.size()};
  context.audioBeat = true;
  context.rng = avs::core::DeterministicRng(1337);

  // Mirror the RNG sequence to predict the expected offsets.
  avs::core::DeterministicRng expectedRng(1337);
  const int offsetA = static_cast<int>(expectedRng.nextUint32() % 32u) - 6;
  int offsetB = static_cast<int>(expectedRng.nextUint32() % 64u) - 32;
  if (offsetB < 0 && offsetB > -16) {
    offsetB = -32;
  }
  if (offsetB >= 0 && offsetB < 16) {
    offsetB = 32;
  }
  const int offsetC = static_cast<int>(expectedRng.nextUint32() % 32u) - 6;

  ASSERT_TRUE(effect.render(context));

  EXPECT_EQ(pixel[0], static_cast<std::uint8_t>(std::clamp(60 + offsetB, 0, 255)));
  EXPECT_EQ(pixel[1], static_cast<std::uint8_t>(std::clamp(20 + offsetA, 0, 255)));
  EXPECT_EQ(pixel[2], static_cast<std::uint8_t>(std::clamp(10 + offsetC, 0, 255)));
}

TEST(ColorfadeEffect, SmoothingReturnsOffsetsToConfiguredBaseValues) {
  Colorfade smoothEffect;
  Colorfade baseEffect;

  avs::core::ParamBlock baseParams;
  baseParams.setInt("offset_a", 12);
  baseParams.setInt("offset_b", -18);
  baseParams.setInt("offset_c", 7);
  baseEffect.setParams(baseParams);

  avs::core::ParamBlock smoothParams;
  smoothParams.setBool("smooth", true);
  smoothParams.setInt("offset_a", 12);
  smoothParams.setInt("offset_b", -18);
  smoothParams.setInt("offset_c", 7);
  smoothParams.setInt("beat_offset_a", -4);
  smoothParams.setInt("beat_offset_b", 11);
  smoothParams.setInt("beat_offset_c", -9);
  smoothEffect.setParams(smoothParams);

  const std::array<std::uint8_t, 16> initialPixels{
      250u, 20u, 10u, 255u,
      15u, 200u, 10u, 255u,
      5u, 40u, 230u, 255u,
      120u, 120u, 120u, 255u,
  };

  std::array<std::uint8_t, 16> smoothPixels = initialPixels;
  avs::core::RenderContext smoothContext = makeContext(smoothPixels);
  smoothContext.audioBeat = true;
  ASSERT_TRUE(smoothEffect.render(smoothContext));

  smoothPixels = initialPixels;
  smoothContext.audioBeat = false;

  const int iterations = 64;
  for (int i = 0; i < iterations; ++i) {
    ASSERT_TRUE(smoothEffect.render(smoothContext));
    if (i + 1 < iterations) {
      smoothPixels = initialPixels;
    }
  }

  std::array<std::uint8_t, 16> basePixels = initialPixels;
  avs::core::RenderContext baseContext = makeContext(basePixels);
  ASSERT_TRUE(baseEffect.render(baseContext));

  EXPECT_EQ(smoothPixels, basePixels);
}
