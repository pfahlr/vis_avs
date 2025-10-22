#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

#include <avs/core/ParamBlock.hpp>
#include <avs/core/RenderContext.hpp>
#include <avs/effects/legacy/trans/effect_multi_delay.h>

namespace {

constexpr int kWidth = 4;
constexpr int kHeight = 1;
constexpr std::size_t kChannels = 4u;

std::array<std::uint8_t, 4> makeColor(int frame) {
  return {static_cast<std::uint8_t>((frame * 17 + 5) & 0xFF),
          static_cast<std::uint8_t>((frame * 31 + 11) & 0xFF),
          static_cast<std::uint8_t>((frame * 47 + 19) & 0xFF), 255u};
}

void fillFrame(std::vector<std::uint8_t>& buffer, const std::array<std::uint8_t, 4>& color) {
  for (std::size_t i = 0; i + kChannels <= buffer.size(); i += kChannels) {
    std::copy(color.begin(), color.end(), buffer.begin() + static_cast<std::ptrdiff_t>(i));
  }
}

std::array<std::uint8_t, 4> firstPixel(const std::vector<std::uint8_t>& buffer) {
  if (buffer.size() < kChannels) {
    return {0u, 0u, 0u, 0u};
  }
  return {buffer[0], buffer[1], buffer[2], buffer[3]};
}

}  // namespace

TEST(MultiDelayEffectTest, StoresAndFetchesFixedDelay) {
  avs::effects::trans::MultiDelay store;
  avs::effects::trans::MultiDelay fetch;

  avs::core::ParamBlock shared;
  shared.setInt("delay0", 2);
  shared.setInt("usebeat0", 0);
  store.setParams(shared);
  fetch.setParams(shared);

  avs::core::ParamBlock storeParams;
  storeParams.setInt("mode", 1);
  storeParams.setInt("buffer", 0);
  store.setParams(storeParams);

  avs::core::ParamBlock fetchParams;
  fetchParams.setInt("mode", 2);
  fetchParams.setInt("buffer", 0);
  fetch.setParams(fetchParams);

  std::vector<std::uint8_t> framebuffer(
      static_cast<std::size_t>(kWidth) * static_cast<std::size_t>(kHeight) * kChannels, 0u);
  avs::core::RenderContext context{};
  context.width = kWidth;
  context.height = kHeight;
  context.framebuffer = {framebuffer.data(), framebuffer.size()};
  context.deltaSeconds = 1.0 / 60.0;

  std::array<std::array<std::uint8_t, 4>, 6> outputs{};
  const std::array<std::uint8_t, 4> zero{0u, 0u, 0u, 0u};

  for (int frame = 0; frame < 6; ++frame) {
    context.frameIndex = static_cast<std::uint64_t>(frame);
    context.audioBeat = false;
    const auto color = makeColor(frame);
    fillFrame(framebuffer, color);
    ASSERT_TRUE(store.render(context));
    ASSERT_TRUE(fetch.render(context));
    outputs[static_cast<std::size_t>(frame)] = firstPixel(framebuffer);
  }

  EXPECT_EQ(outputs[0], zero);
  EXPECT_EQ(outputs[1], zero);
  EXPECT_EQ(outputs[2], makeColor(0));
  EXPECT_EQ(outputs[3], makeColor(1));
  EXPECT_EQ(outputs[4], makeColor(2));
  EXPECT_EQ(outputs[5], makeColor(3));
}

TEST(MultiDelayEffectTest, BeatSynchronizedDelayUsesBeatInterval) {
  avs::effects::trans::MultiDelay store;
  avs::effects::trans::MultiDelay fetch;

  avs::core::ParamBlock shared;
  shared.setInt("usebeat0", 1);
  store.setParams(shared);
  fetch.setParams(shared);

  avs::core::ParamBlock storeParams;
  storeParams.setInt("mode", 1);
  storeParams.setInt("buffer", 0);
  store.setParams(storeParams);

  avs::core::ParamBlock fetchParams;
  fetchParams.setInt("mode", 2);
  fetchParams.setInt("buffer", 0);
  fetch.setParams(fetchParams);

  std::vector<std::uint8_t> framebuffer(
      static_cast<std::size_t>(kWidth) * static_cast<std::size_t>(kHeight) * kChannels, 0u);
  avs::core::RenderContext context{};
  context.width = kWidth;
  context.height = kHeight;
  context.framebuffer = {framebuffer.data(), framebuffer.size()};
  context.deltaSeconds = 1.0 / 60.0;

  std::array<std::array<std::uint8_t, 4>, 8> outputs{};
  const std::array<std::uint8_t, 4> zero{0u, 0u, 0u, 0u};

  for (int frame = 0; frame < 8; ++frame) {
    context.frameIndex = static_cast<std::uint64_t>(frame);
    context.audioBeat = (frame % 3) == 0;
    const auto color = makeColor(frame);
    fillFrame(framebuffer, color);
    ASSERT_TRUE(store.render(context));
    ASSERT_TRUE(fetch.render(context));
    outputs[static_cast<std::size_t>(frame)] = firstPixel(framebuffer);
  }

  EXPECT_EQ(outputs[0], makeColor(0));
  EXPECT_EQ(outputs[1], makeColor(1));
  EXPECT_EQ(outputs[2], makeColor(2));
  EXPECT_EQ(outputs[3], zero);
  EXPECT_EQ(outputs[4], zero);
  EXPECT_EQ(outputs[5], zero);
  EXPECT_EQ(outputs[6], makeColor(3));
  EXPECT_EQ(outputs[7], makeColor(4));
}

TEST(MultiDelayEffectTest, ResetsWhenFrameSizeChanges) {
  avs::effects::trans::MultiDelay store;
  avs::effects::trans::MultiDelay fetch;

  avs::core::ParamBlock shared;
  shared.setInt("delay0", 2);
  store.setParams(shared);
  fetch.setParams(shared);

  avs::core::ParamBlock storeParams;
  storeParams.setInt("mode", 1);
  storeParams.setInt("buffer", 0);
  store.setParams(storeParams);

  avs::core::ParamBlock fetchParams;
  fetchParams.setInt("mode", 2);
  fetchParams.setInt("buffer", 0);
  fetch.setParams(fetchParams);

  std::vector<std::uint8_t> bufferA(
      static_cast<std::size_t>(kWidth) * static_cast<std::size_t>(kHeight) * kChannels, 0u);
  avs::core::RenderContext context{};
  context.width = kWidth;
  context.height = kHeight;
  context.framebuffer = {bufferA.data(), bufferA.size()};
  context.deltaSeconds = 1.0 / 60.0;

  std::array<std::array<std::uint8_t, 4>, 5> outputs{};
  const std::array<std::uint8_t, 4> zero{0u, 0u, 0u, 0u};

  for (int frame = 0; frame < 3; ++frame) {
    context.frameIndex = static_cast<std::uint64_t>(frame);
    context.audioBeat = false;
    fillFrame(bufferA, makeColor(frame));
    ASSERT_TRUE(store.render(context));
    ASSERT_TRUE(fetch.render(context));
    outputs[static_cast<std::size_t>(frame)] = firstPixel(bufferA);
  }

  std::vector<std::uint8_t> bufferB(
      static_cast<std::size_t>(2) * static_cast<std::size_t>(2) * kChannels, 0u);
  context.width = 2;
  context.height = 2;
  context.framebuffer = {bufferB.data(), bufferB.size()};

  for (int frame = 3; frame < 5; ++frame) {
    context.frameIndex = static_cast<std::uint64_t>(frame);
    context.audioBeat = false;
    fillFrame(bufferB, makeColor(frame));
    ASSERT_TRUE(store.render(context));
    ASSERT_TRUE(fetch.render(context));
    outputs[static_cast<std::size_t>(frame)] = firstPixel(bufferB);
  }

  EXPECT_EQ(outputs[0], zero);
  EXPECT_EQ(outputs[1], zero);
  EXPECT_EQ(outputs[2], makeColor(0));
  EXPECT_EQ(outputs[3], zero);
  EXPECT_EQ(outputs[4], zero);
}
