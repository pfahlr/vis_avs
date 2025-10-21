#include <gtest/gtest.h>

#include <array>
#include <cstdint>

#include "avs/core/DeterministicRng.hpp"
#include "avs/core/RenderContext.hpp"
#include "effects/trans/effect_channel_shift.h"

namespace {

constexpr int kIdRgb = 1183;
constexpr int kIdRbg = 1020;

using avs::effects::trans::ChannelShift;

avs::core::RenderContext makeContext(std::array<std::uint8_t, 4>& pixel) {
  avs::core::RenderContext context{};
  context.width = 1;
  context.height = 1;
  context.framebuffer = {pixel.data(), pixel.size()};
  return context;
}

}  // namespace

TEST(ChannelShiftEffect, DefaultModeSwapsGreenAndBlue) {
  ChannelShift effect;
  std::array<std::uint8_t, 4> pixel{10u, 20u, 30u, 255u};
  auto context = makeContext(pixel);

  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(pixel[0], 10u);
  EXPECT_EQ(pixel[1], 30u);
  EXPECT_EQ(pixel[2], 20u);
  EXPECT_EQ(pixel[3], 255u);
}

TEST(ChannelShiftEffect, ModeParameterDisablesPermutation) {
  ChannelShift effect;
  avs::core::ParamBlock params;
  params.setInt("mode", kIdRgb);
  params.setBool("onbeat", false);
  effect.setParams(params);

  std::array<std::uint8_t, 4> pixel{42u, 11u, 99u, 7u};
  auto context = makeContext(pixel);
  context.audioBeat = true;

  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(pixel[0], 42u);
  EXPECT_EQ(pixel[1], 11u);
  EXPECT_EQ(pixel[2], 99u);
  EXPECT_EQ(pixel[3], 7u);
}

TEST(ChannelShiftEffect, BeatRandomisationFollowsDeterministicRng) {
  ChannelShift effect;
  avs::core::ParamBlock params;
  params.setInt("mode", kIdRgb);
  params.setBool("onbeat", true);
  effect.setParams(params);

  std::array<std::uint8_t, 4> pixel{1u, 2u, 3u, 255u};
  auto context = makeContext(pixel);
  context.audioBeat = true;

  avs::core::DeterministicRng rng(1234);
  rng.reseed(0);
  avs::core::DeterministicRng clone = rng;
  const std::uint32_t randomValue = rng.nextUint32();
  context.rng = clone;

  const std::array<std::uint8_t, 3> original = {pixel[0], pixel[1], pixel[2]};

  ASSERT_TRUE(effect.render(context));

  const std::array<std::array<std::uint8_t, 3>, 6> orders = {{{0u, 1u, 2u}, {0u, 2u, 1u}, {1u, 2u, 0u},
                                                             {1u, 0u, 2u}, {2u, 0u, 1u}, {2u, 1u, 0u}}};
  const std::array<std::uint8_t, 3>& expectedOrder = orders[randomValue % orders.size()];

  EXPECT_EQ(pixel[0], original[expectedOrder[0]]);
  EXPECT_EQ(pixel[1], original[expectedOrder[1]]);
  EXPECT_EQ(pixel[2], original[expectedOrder[2]]);
  EXPECT_EQ(pixel[3], 255u);

  std::array<std::uint8_t, 4> secondPixel{9u, 8u, 7u, 0u};
  context = makeContext(secondPixel);
  context.audioBeat = false;

  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(secondPixel[0], (std::array<std::uint8_t, 3>{9u, 8u, 7u})[expectedOrder[0]]);
  EXPECT_EQ(secondPixel[1], (std::array<std::uint8_t, 3>{9u, 8u, 7u})[expectedOrder[1]]);
  EXPECT_EQ(secondPixel[2], (std::array<std::uint8_t, 3>{9u, 8u, 7u})[expectedOrder[2]]);
  EXPECT_EQ(secondPixel[3], 0u);
}

TEST(ChannelShiftEffect, OrderStringParameterOverridesModeId) {
  ChannelShift effect;
  avs::core::ParamBlock params;
  params.setInt("mode", kIdRgb);
  params.setString("order", "gbr");
  params.setBool("onbeat", false);
  effect.setParams(params);

  std::array<std::uint8_t, 4> pixel{100u, 150u, 200u, 255u};
  auto context = makeContext(pixel);

  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(pixel[0], 150u);
  EXPECT_EQ(pixel[1], 200u);
  EXPECT_EQ(pixel[2], 100u);
  EXPECT_EQ(pixel[3], 255u);
}

TEST(ChannelShiftEffect, DisablingBeatRestoresConfiguredMode) {
  ChannelShift effect;
  avs::core::ParamBlock params;
  params.setInt("mode", kIdRbg);
  params.setBool("onbeat", true);
  effect.setParams(params);

  std::array<std::uint8_t, 4> pixel{5u, 15u, 25u, 255u};
  auto context = makeContext(pixel);
  context.audioBeat = true;

  avs::core::DeterministicRng rng(42);
  rng.reseed(0);
  context.rng = rng;

  ASSERT_TRUE(effect.render(context));

  params.setBool("onbeat", false);
  effect.setParams(params);

  std::array<std::uint8_t, 4> secondPixel{9u, 19u, 29u, 255u};
  context = makeContext(secondPixel);
  context.audioBeat = false;

  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(secondPixel[0], 9u);
  EXPECT_EQ(secondPixel[1], 29u);
  EXPECT_EQ(secondPixel[2], 19u);
  EXPECT_EQ(secondPixel[3], 255u);
}

