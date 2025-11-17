#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

#include <avs/core/ParamBlock.hpp>
#include <avs/core/RenderContext.hpp>
#include <avs/effects/trans/effect_mosaic.h>

namespace {

avs::core::RenderContext makeContext(std::vector<std::uint8_t>& buffer, int width, int height) {
  avs::core::RenderContext context{};
  context.width = width;
  context.height = height;
  context.framebuffer = {buffer.data(), buffer.size()};
  context.audioBeat = false;
  return context;
}

std::vector<std::uint8_t> makeSequentialPattern(int width, int height) {
  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width) *
                                   static_cast<std::size_t>(height) * 4u);
  auto* words = reinterpret_cast<std::uint32_t*>(pixels.data());
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const std::size_t index = static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                                static_cast<std::size_t>(x);
      words[index] = static_cast<std::uint32_t>((index + 1u) * 0x01010101u);
    }
  }
  return pixels;
}

std::vector<std::uint8_t> runReferenceMosaic(const std::vector<std::uint8_t>& base,
                                             int width,
                                             int height,
                                             int quality) {
  constexpr int kOne = 1 << 16;
  std::vector<std::uint8_t> result = base;
  const auto* source = reinterpret_cast<const std::uint32_t*>(base.data());
  auto* dst = reinterpret_cast<std::uint32_t*>(result.data());

  const int sXInc = (width * 65536) / quality;
  const int sYInc = (height * 65536) / quality;
  int ypos = sYInc >> 17;
  int dypos = 0;

  for (int y = 0; y < height; ++y) {
    if (ypos >= height) {
      break;
    }
    const int sampleRow = ypos * width;
    int dpos = 0;
    int xpos = sXInc >> 17;
    if (xpos >= width) {
      xpos = width - 1;
    }
    std::uint32_t sample = source[sampleRow + xpos];
    for (int x = 0; x < width; ++x) {
      dst[static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
          static_cast<std::size_t>(x)] = sample;
      dpos += sXInc;
      if (dpos >= kOne) {
        const int advance = dpos >> 16;
        xpos += advance;
        if (xpos >= width) {
          break;
        }
        sample = source[sampleRow + xpos];
        dpos -= advance * kOne;
      }
    }

    dypos += sYInc;
    if (dypos >= kOne) {
      const int advance = dypos >> 16;
      ypos += advance;
      if (ypos >= height) {
        break;
      }
      dypos -= advance * kOne;
    }
  }

  return result;
}

TEST(MosaicEffect, ReplaceModeUsesBlockSample) {
  constexpr int kWidth = 4;
  constexpr int kHeight = 4;
  auto base = makeSequentialPattern(kWidth, kHeight);
  auto working = base;

  avs::effects::trans::Mosaic effect;
  avs::core::ParamBlock params;
  params.setBool("enabled", true);
  params.setInt("quality", 1);
  effect.setParams(params);

  auto context = makeContext(working, kWidth, kHeight);
  ASSERT_TRUE(effect.render(context));

  const auto* output = reinterpret_cast<const std::uint32_t*>(working.data());
  const auto* input = reinterpret_cast<const std::uint32_t*>(base.data());
  const std::uint32_t expected = input[static_cast<std::size_t>(2 * kWidth + 2)];
  for (int i = 0; i < kWidth * kHeight; ++i) {
    EXPECT_EQ(output[i], expected);
  }
}

TEST(MosaicEffect, AdditiveBlendSaturatesChannels) {
  std::vector<std::uint8_t> pixels(4u);
  const std::uint32_t basePixel = 0xF0E0D0C0u;
  std::memcpy(pixels.data(), &basePixel, sizeof(basePixel));
  auto working = pixels;

  avs::effects::trans::Mosaic effect;
  avs::core::ParamBlock params;
  params.setBool("enabled", true);
  params.setBool("blend", true);
  params.setInt("quality", 1);
  effect.setParams(params);

  auto context = makeContext(working, 1, 1);
  ASSERT_TRUE(effect.render(context));

  std::uint32_t result = 0;
  std::memcpy(&result, working.data(), sizeof(result));
  EXPECT_EQ(result, 0xFFFFFFFFu);
}

TEST(MosaicEffect, BeatLatchMatchesLegacyStepDown) {
  constexpr int kWidth = 4;
  constexpr int kHeight = 4;
  auto base = makeSequentialPattern(kWidth, kHeight);
  auto working = base;

  avs::effects::trans::Mosaic effect;
  avs::core::ParamBlock params;
  params.setBool("enabled", true);
  params.setInt("quality", 80);
  params.setInt("quality_onbeat", 10);
  params.setBool("on_beat", true);
  params.setInt("beat_duration", 4);
  effect.setParams(params);

  auto context = makeContext(working, kWidth, kHeight);

  const std::array<int, 5> expectedQualities{10, 27, 44, 61, 80};
  for (std::size_t frame = 0; frame < expectedQualities.size(); ++frame) {
    std::copy(base.begin(), base.end(), working.begin());
    context.framebuffer = {working.data(), working.size()};
    context.frameIndex = frame;
    context.audioBeat = (frame == 0);

    ASSERT_TRUE(effect.render(context));

    avs::effects::trans::Mosaic reference;
    avs::core::ParamBlock referenceParams;
    referenceParams.setBool("enabled", true);
    referenceParams.setInt("quality", expectedQualities[frame]);
    reference.setParams(referenceParams);

    auto expected = base;
    auto expectedContext = makeContext(expected, kWidth, kHeight);
    expectedContext.frameIndex = frame;
    ASSERT_TRUE(reference.render(expectedContext));

    EXPECT_EQ(working, expected) << "frame " << frame;
  }
}

TEST(MosaicEffect, HighQualityProcessesEntireFrame) {
  constexpr int kWidth = 10;
  constexpr int kHeight = 6;
  constexpr int kQuality = 20;

  auto base = makeSequentialPattern(kWidth, kHeight);
  auto working = base;

  avs::effects::trans::Mosaic effect;
  avs::core::ParamBlock params;
  params.setBool("enabled", true);
  params.setInt("quality", kQuality);
  effect.setParams(params);

  auto context = makeContext(working, kWidth, kHeight);
  ASSERT_TRUE(effect.render(context));

  auto expected = runReferenceMosaic(base, kWidth, kHeight, kQuality);

  EXPECT_EQ(working, expected);
}

}  // namespace
