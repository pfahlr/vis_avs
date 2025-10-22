#include <gtest/gtest.h>

#include <array>
#include <cstdint>

#include <avs/core/ParamBlock.hpp>
#include <avs/core/RenderContext.hpp>
#include <avs/effects/legacy/trans/effect_brightness.h>

namespace {

avs::core::RenderContext makeContext(std::array<std::uint8_t, 8>& pixels) {
  avs::core::RenderContext context{};
  context.width = 2;
  context.height = 1;
  context.framebuffer = {pixels.data(), pixels.size()};
  return context;
}

}  // namespace

TEST(TransBrightnessEffect, AppliesChannelScalingAndClamp) {
  avs::effects::trans::Brightness effect;
  avs::core::ParamBlock params;
  params.setBool("enabled", true);
  params.setBool("blend", false);
  params.setBool("blendavg", false);
  params.setInt("redp", 1024);     // multiplier = 5x
  params.setInt("greenp", -2048);  // multiplier = 0.5x
  params.setInt("bluep", 4096);    // multiplier = 17x
  effect.setParams(params);

  std::array<std::uint8_t, 8> pixels{50u, 120u, 20u, 255u, 200u, 40u, 10u, 255u};
  auto context = makeContext(pixels);

  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(pixels[0], 250u);  // 50 * 5 = 250
  EXPECT_EQ(pixels[1], 60u);   // 120 * 0.5 = 60
  EXPECT_EQ(pixels[2], 255u);  // 20 * 17 clamps to 255
  EXPECT_EQ(pixels[4], 255u);  // 200 * 5 clamps to 255
  EXPECT_EQ(pixels[5], 20u);   // 40 * 0.5 = 20
  EXPECT_EQ(pixels[6], 170u);  // 10 * 17 = 170
  EXPECT_EQ(pixels[3], 255u);
  EXPECT_EQ(pixels[7], 255u);
}

TEST(TransBrightnessEffect, SupportsAdditiveBlendMode) {
  avs::effects::trans::Brightness effect;
  avs::core::ParamBlock params;
  params.setBool("enabled", true);
  params.setBool("blend", true);
  params.setBool("blendavg", false);
  params.setInt("redp", 0);
  params.setInt("greenp", 0);
  params.setInt("bluep", 0);
  effect.setParams(params);

  std::array<std::uint8_t, 8> pixels{100u, 120u, 130u, 255u, 160u, 200u, 220u, 255u};
  auto context = makeContext(pixels);

  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(pixels[0], 200u);  // 100 + 100
  EXPECT_EQ(pixels[1], 240u);  // 120 + 120
  EXPECT_EQ(pixels[2], 255u);  // 130 + 130 clamps to 255
  EXPECT_EQ(pixels[4], 255u);  // 160 + 160 clamps to 255
  EXPECT_EQ(pixels[5], 255u);  // 200 + 200 clamps to 255
  EXPECT_EQ(pixels[6], 255u);  // 220 + 220 clamps to 255
}

TEST(TransBrightnessEffect, SupportsAverageBlendMode) {
  avs::effects::trans::Brightness effect;
  avs::core::ParamBlock params;
  params.setBool("enabled", true);
  params.setBool("blend", false);
  params.setBool("blendavg", true);
  params.setInt("redp", 2048);  // multiplier = 9x
  params.setInt("greenp", 0);
  params.setInt("bluep", -4096);  // multiplier = 0x
  effect.setParams(params);

  std::array<std::uint8_t, 8> pixels{20u, 60u, 200u, 255u, 40u, 100u, 80u, 255u};
  auto context = makeContext(pixels);

  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(pixels[0], 100u);  // (20 + 180) / 2
  EXPECT_EQ(pixels[1], 60u);   // (60 + 60) / 2
  EXPECT_EQ(pixels[2], 100u);  // (200 + 0) / 2
  EXPECT_EQ(pixels[4], 147u);  // (40 + 255) / 2
  EXPECT_EQ(pixels[5], 100u);  // (100 + 100) / 2
  EXPECT_EQ(pixels[6], 40u);   // (80 + 0) / 2
}

TEST(TransBrightnessEffect, HonorsExclusionMask) {
  avs::effects::trans::Brightness effect;
  avs::core::ParamBlock params;
  params.setBool("enabled", true);
  params.setBool("blend", false);
  params.setBool("blendavg", false);
  params.setBool("exclude", true);
  params.setInt("color", 0x123456);
  params.setInt("distance", 4);
  params.setInt("redp", 4096);
  params.setInt("greenp", 4096);
  params.setInt("bluep", 4096);
  effect.setParams(params);

  std::array<std::uint8_t, 8> pixels{0x12u, 0x34u, 0x56u, 255u, 0x30u, 0x10u, 0x08u, 255u};
  auto context = makeContext(pixels);

  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(pixels[0], 0x12u);
  EXPECT_EQ(pixels[1], 0x34u);
  EXPECT_EQ(pixels[2], 0x56u);
  EXPECT_EQ(pixels[4], 255u);
  EXPECT_EQ(pixels[5], 255u);
  EXPECT_EQ(pixels[6], 136u);  // 0x08 * 17
}
