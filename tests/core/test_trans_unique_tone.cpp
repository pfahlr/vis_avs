#include <gtest/gtest.h>

#include <array>
#include <cstdint>

#include <avs/core/ParamBlock.hpp>
#include <avs/core/RenderContext.hpp>
#include <avs/effects/legacy/trans/effect_unique_tone.h>

namespace {

avs::core::RenderContext makeContext(std::array<std::uint8_t, 8>& pixels) {
  avs::core::RenderContext context{};
  context.width = 2;
  context.height = 1;
  context.framebuffer = {pixels.data(), pixels.size()};
  return context;
}

}  // namespace

TEST(TransUniqueToneEffect, FlattensToTargetTone) {
  avs::effects::trans::UniqueTone effect;
  avs::core::ParamBlock params;
  params.setBool("enabled", true);
  params.setBool("blend", false);
  params.setBool("blendavg", false);
  params.setInt("color", 0x804020);
  effect.setParams(params);

  std::array<std::uint8_t, 8> pixels{100u, 50u, 25u, 255u, 220u, 30u, 10u, 255u};
  auto context = makeContext(pixels);

  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(pixels[0], 50u);
  EXPECT_EQ(pixels[1], 25u);
  EXPECT_EQ(pixels[2], 12u);
  EXPECT_EQ(pixels[4], 110u);
  EXPECT_EQ(pixels[5], 55u);
  EXPECT_EQ(pixels[6], 27u);
}

TEST(TransUniqueToneEffect, SupportsAdditiveBlend) {
  avs::effects::trans::UniqueTone effect;
  avs::core::ParamBlock params;
  params.setBool("enabled", true);
  params.setBool("blend", true);
  params.setBool("blendavg", false);
  params.setInt("color", 0xFF0000);
  effect.setParams(params);

  std::array<std::uint8_t, 8> pixels{100u, 20u, 10u, 255u, 200u, 100u, 50u, 255u};
  auto context = makeContext(pixels);

  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(pixels[0], 200u);
  EXPECT_EQ(pixels[1], 20u);
  EXPECT_EQ(pixels[2], 10u);
  EXPECT_EQ(pixels[4], 255u);
  EXPECT_EQ(pixels[5], 100u);
  EXPECT_EQ(pixels[6], 50u);
}

TEST(TransUniqueToneEffect, SupportsInvertAndAverageBlend) {
  avs::effects::trans::UniqueTone effect;
  avs::core::ParamBlock params;
  params.setBool("enabled", true);
  params.setBool("blend", false);
  params.setBool("blendavg", true);
  params.setBool("invert", true);
  params.setInt("color", 0x00FF00);
  effect.setParams(params);

  std::array<std::uint8_t, 8> pixels{200u, 150u, 100u, 255u, 50u, 60u, 70u, 255u};
  auto context = makeContext(pixels);

  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(pixels[0], 100u);
  EXPECT_EQ(pixels[1], 102u);
  EXPECT_EQ(pixels[2], 50u);
  EXPECT_EQ(pixels[4], 25u);
  EXPECT_EQ(pixels[5], 122u);
  EXPECT_EQ(pixels[6], 35u);
}
