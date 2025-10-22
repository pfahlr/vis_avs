#include <gtest/gtest.h>

#include <array>
#include <cstdint>

#include "avs/core/ParamBlock.hpp"
#include "avs/core/RenderContext.hpp"
#include "effects/trans/effect_multiplier.h"

namespace {

avs::core::RenderContext makeContext(std::array<std::uint8_t, 8>& pixels) {
  avs::core::RenderContext context{};
  context.width = 2;
  context.height = 1;
  context.framebuffer = {pixels.data(), pixels.size()};
  return context;
}

}  // namespace

TEST(TransMultiplierEffect, DoublesWithSaturation) {
  avs::effects::trans::Multiplier effect;
  avs::core::ParamBlock params;
  params.setInt("mode", 3);  // ×2
  effect.setParams(params);

  std::array<std::uint8_t, 8> pixels{60u, 120u, 200u, 255u, 10u, 40u, 90u, 255u};
  auto context = makeContext(pixels);

  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(pixels[0], 120u);
  EXPECT_EQ(pixels[1], 240u);
  EXPECT_EQ(pixels[2], 255u);
  EXPECT_EQ(pixels[4], 20u);
  EXPECT_EQ(pixels[5], 80u);
  EXPECT_EQ(pixels[6], 180u);
  EXPECT_EQ(pixels[3], 255u);
  EXPECT_EQ(pixels[7], 255u);
}

TEST(TransMultiplierEffect, InfinityModePromotesNonBlackToWhite) {
  avs::effects::trans::Multiplier effect;
  avs::core::ParamBlock params;
  params.setInt("mode", 0);
  effect.setParams(params);

  std::array<std::uint8_t, 8> pixels{0u, 0u, 0u, 255u, 1u, 2u, 3u, 10u};
  auto context = makeContext(pixels);

  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(pixels[0], 0u);
  EXPECT_EQ(pixels[1], 0u);
  EXPECT_EQ(pixels[2], 0u);
  EXPECT_EQ(pixels[4], 255u);
  EXPECT_EQ(pixels[5], 255u);
  EXPECT_EQ(pixels[6], 255u);
  EXPECT_EQ(pixels[3], 255u);
  EXPECT_EQ(pixels[7], 10u);
}

TEST(TransMultiplierEffect, ZeroModePreservesPureWhite) {
  avs::effects::trans::Multiplier effect;
  avs::core::ParamBlock params;
  params.setInt("mode", 7);
  effect.setParams(params);

  std::array<std::uint8_t, 8> pixels{255u, 255u, 255u, 10u, 255u, 250u, 255u, 20u};
  auto context = makeContext(pixels);

  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(pixels[0], 255u);
  EXPECT_EQ(pixels[1], 255u);
  EXPECT_EQ(pixels[2], 255u);
  EXPECT_EQ(pixels[4], 0u);
  EXPECT_EQ(pixels[5], 0u);
  EXPECT_EQ(pixels[6], 0u);
  EXPECT_EQ(pixels[3], 10u);
  EXPECT_EQ(pixels[7], 20u);
}

TEST(TransMultiplierEffect, CustomFactorsOverrideMode) {
  avs::effects::trans::Multiplier effect;
  avs::core::ParamBlock params;
  params.setInt("mode", 1);  // ×8 but will be overridden
  params.setFloat("factor_r", 0.5f);
  params.setFloat("factor_g", 1.0f);
  params.setFloat("factor_b", 1.5f);
  effect.setParams(params);

  std::array<std::uint8_t, 8> pixels{100u, 50u, 10u, 99u, 40u, 60u, 80u, 77u};
  auto context = makeContext(pixels);

  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(pixels[0], 50u);
  EXPECT_EQ(pixels[1], 50u);
  EXPECT_EQ(pixels[2], 15u);
  EXPECT_EQ(pixels[4], 20u);
  EXPECT_EQ(pixels[5], 60u);
  EXPECT_EQ(pixels[6], 120u);
  EXPECT_EQ(pixels[3], 99u);
  EXPECT_EQ(pixels[7], 77u);
}

TEST(TransMultiplierEffect, SingleChannelDefaultsOthersToNeutral) {
  avs::effects::trans::Multiplier effect;
  avs::core::ParamBlock params;
  params.setFloat("factor_r", 0.5f);
  effect.setParams(params);

  std::array<std::uint8_t, 8> pixels{100u, 50u, 10u, 0u, 20u, 40u, 80u, 0u};
  auto context = makeContext(pixels);

  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(pixels[0], 50u);
  EXPECT_EQ(pixels[1], 50u);
  EXPECT_EQ(pixels[2], 10u);
  EXPECT_EQ(pixels[4], 10u);
  EXPECT_EQ(pixels[5], 40u);
  EXPECT_EQ(pixels[6], 80u);
}

TEST(TransMultiplierEffect, ChannelOverrideRetainsPreviousUniformFactor) {
  avs::effects::trans::Multiplier effect;
  avs::core::ParamBlock params;
  params.setFloat("factor", 1.5f);
  effect.setParams(params);

  avs::core::ParamBlock overrideParams;
  overrideParams.setFloat("factor_r", 0.5f);
  effect.setParams(overrideParams);

  std::array<std::uint8_t, 8> pixels{100u, 80u, 60u, 0u, 40u, 20u, 10u, 0u};
  auto context = makeContext(pixels);

  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(pixels[0], 50u);
  EXPECT_EQ(pixels[1], 120u);
  EXPECT_EQ(pixels[2], 90u);
  EXPECT_EQ(pixels[4], 20u);
  EXPECT_EQ(pixels[5], 30u);
  EXPECT_EQ(pixels[6], 15u);
}
