#include <array>
#include <cstdint>

#include "avs/core/ParamBlock.hpp"
#include "avs/core/RenderContext.hpp"
#include "effects/trans/effect_video_delay.h"
#include "gtest/gtest.h"

namespace {

using avs::effects::trans::VideoDelay;

avs::core::RenderContext makeContext(std::uint8_t* data, std::size_t size) {
  avs::core::RenderContext context{};
  context.width = 1;
  context.height = 1;
  context.framebuffer = {data, size};
  return context;
}

TEST(VideoDelayEffect, MaintainsFixedFrameDelay) {
  VideoDelay effect;
  avs::core::ParamBlock params;
  params.setBool("enabled", true);
  params.setBool("use_beats", false);
  params.setInt("delay", 2);
  effect.setParams(params);

  std::array<std::uint8_t, 4> pixel{10u, 20u, 30u, 255u};
  auto context = makeContext(pixel.data(), pixel.size());

  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(pixel[0], 0u);
  EXPECT_EQ(pixel[1], 0u);
  EXPECT_EQ(pixel[2], 0u);
  EXPECT_EQ(pixel[3], 0u);

  pixel = {50u, 60u, 70u, 255u};
  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(pixel[0], 0u);
  EXPECT_EQ(pixel[1], 0u);
  EXPECT_EQ(pixel[2], 0u);
  EXPECT_EQ(pixel[3], 0u);

  pixel = {90u, 100u, 110u, 255u};
  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(pixel[0], 10u);
  EXPECT_EQ(pixel[1], 20u);
  EXPECT_EQ(pixel[2], 30u);
  EXPECT_EQ(pixel[3], 255u);

  pixel = {130u, 140u, 150u, 255u};
  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(pixel[0], 50u);
  EXPECT_EQ(pixel[1], 60u);
  EXPECT_EQ(pixel[2], 70u);
  EXPECT_EQ(pixel[3], 255u);
}

TEST(VideoDelayEffect, PreservesStoredAlpha) {
  VideoDelay effect;
  avs::core::ParamBlock params;
  params.setBool("use_beats", false);
  params.setInt("delay", 1);
  effect.setParams(params);

  std::array<std::uint8_t, 4> pixel{200u, 0u, 0u, 128u};
  auto context = makeContext(pixel.data(), pixel.size());

  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(pixel[0], 0u);
  EXPECT_EQ(pixel[1], 0u);
  EXPECT_EQ(pixel[2], 0u);
  EXPECT_EQ(pixel[3], 0u);

  pixel = {0u, 0u, 200u, 255u};
  ASSERT_TRUE(effect.render(context));

  EXPECT_EQ(pixel[0], 200u);
  EXPECT_EQ(pixel[1], 0u);
  EXPECT_EQ(pixel[2], 0u);
  EXPECT_EQ(pixel[3], 128u);
}

TEST(VideoDelayEffect, BeatModeUpdatesDelay) {
  VideoDelay effect;
  avs::core::ParamBlock params;
  params.setBool("enabled", true);
  params.setBool("use_beats", true);
  params.setInt("delay", 3);
  effect.setParams(params);

  std::array<std::uint8_t, 4> pixel{0u, 0u, 0u, 255u};
  auto context = makeContext(pixel.data(), pixel.size());

  context.audioBeat = true;
  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(effect.currentDelayFrames(), 0);

  context.audioBeat = false;
  pixel = {5u, 5u, 5u, 255u};
  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(effect.currentDelayFrames(), 0);

  pixel = {10u, 10u, 10u, 255u};
  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(effect.currentDelayFrames(), 0);

  context.audioBeat = true;
  pixel = {15u, 15u, 15u, 255u};
  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(effect.currentDelayFrames(), 9);

  context.audioBeat = false;
  pixel = {20u, 20u, 20u, 255u};
  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(effect.currentDelayFrames(), 9);
}

TEST(VideoDelayEffect, BeatModeCapturesPreBeatHistory) {
  VideoDelay effect;
  avs::core::ParamBlock params;
  params.setBool("enabled", true);
  params.setBool("use_beats", true);
  params.setInt("delay", 1);
  effect.setParams(params);

  std::array<std::uint8_t, 4> pixel{0u, 0u, 0u, 255u};
  auto context = makeContext(pixel.data(), pixel.size());

  context.audioBeat = false;
  pixel = {10u, 0u, 0u, 255u};
  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(pixel[0], 10u);
  EXPECT_EQ(pixel[1], 0u);
  EXPECT_EQ(pixel[2], 0u);
  EXPECT_EQ(pixel[3], 255u);

  pixel = {20u, 0u, 0u, 255u};
  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(pixel[0], 20u);
  EXPECT_EQ(pixel[1], 0u);
  EXPECT_EQ(pixel[2], 0u);
  EXPECT_EQ(pixel[3], 255u);

  pixel = {30u, 0u, 0u, 255u};
  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(pixel[0], 30u);
  EXPECT_EQ(pixel[1], 0u);
  EXPECT_EQ(pixel[2], 0u);
  EXPECT_EQ(pixel[3], 255u);

  context.audioBeat = true;
  pixel = {40u, 0u, 0u, 255u};
  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(pixel[0], 10u);
  EXPECT_EQ(pixel[1], 0u);
  EXPECT_EQ(pixel[2], 0u);
  EXPECT_EQ(pixel[3], 255u);
  EXPECT_EQ(effect.currentDelayFrames(), 3);

  context.audioBeat = false;
  pixel = {50u, 0u, 0u, 255u};
  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(pixel[0], 20u);
  EXPECT_EQ(pixel[1], 0u);
  EXPECT_EQ(pixel[2], 0u);
  EXPECT_EQ(pixel[3], 255u);
}

TEST(VideoDelayEffect, ClampsBeatDelayToHistoryLimit) {
  VideoDelay effect;
  avs::core::ParamBlock params;
  params.setBool("enabled", true);
  params.setBool("use_beats", true);
  params.setInt("delay", 16);
  effect.setParams(params);

  std::array<std::uint8_t, 4> pixel{0u, 0u, 0u, 255u};
  auto context = makeContext(pixel.data(), pixel.size());

  context.audioBeat = true;
  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(effect.currentDelayFrames(), 0);

  context.audioBeat = false;
  for (int i = 0; i < 24; ++i) {
    pixel = {static_cast<std::uint8_t>(i + 1), 0u, 0u, 255u};
    ASSERT_TRUE(effect.render(context));
    EXPECT_LE(effect.currentDelayFrames(), 400);
  }

  context.audioBeat = true;
  pixel = {255u, 255u, 255u, 255u};
  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(effect.currentDelayFrames(), 400);
}

}  // namespace
