#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <vector>

#include <avs/core/ParamBlock.hpp>
#include <avs/core/RenderContext.hpp>
#include <avs/effects/legacy/trans/effect_roto_blitter.h>

namespace {

avs::core::RenderContext makeContext(std::vector<std::uint8_t>& pixels, int width, int height) {
  avs::core::RenderContext ctx{};
  ctx.width = width;
  ctx.height = height;
  ctx.framebuffer = {pixels.data(), pixels.size()};
  ctx.audioBeat = false;
  return ctx;
}

void fillPattern(std::vector<std::uint8_t>& pixels, int width, int height) {
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const std::size_t offset =
          (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)) * 4u;
      pixels[offset + 0] = static_cast<std::uint8_t>(x * 32 + y * 4);
      pixels[offset + 1] = static_cast<std::uint8_t>(y * 32 + x * 4);
      pixels[offset + 2] = static_cast<std::uint8_t>((x + y) * 16);
      pixels[offset + 3] = 255;
    }
  }
}

std::array<std::uint8_t, 4> readPixel(const std::vector<std::uint8_t>& pixels, int width, int x, int y) {
  const std::size_t offset =
      (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)) * 4u;
  return {pixels[offset + 0], pixels[offset + 1], pixels[offset + 2], pixels[offset + 3]};
}

}  // namespace

TEST(RotoBlitter, CopiesFrameWhenIdentity) {
  avs::effects::trans::RotoBlitter effect;
  avs::core::ParamBlock params;
  params.setInt("zoom_scale", 31);
  params.setInt("rot_dir", 32);
  params.setBool("blend", false);
  params.setBool("subpixel", false);
  effect.setParams(params);

  constexpr int kWidth = 4;
  constexpr int kHeight = 4;
  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(kWidth * kHeight * 4), 0);
  fillPattern(pixels, kWidth, kHeight);
  std::vector<std::uint8_t> original = pixels;

  auto ctx = makeContext(pixels, kWidth, kHeight);
  ASSERT_TRUE(effect.render(ctx));

  EXPECT_EQ(pixels, original);
}

TEST(RotoBlitter, AnchorPreservesPivotPixel) {
  avs::effects::trans::RotoBlitter effect;
  avs::core::ParamBlock params;
  params.setInt("rot_dir", 32);
  params.setInt("zoom_scale", 31);
  params.setBool("subpixel", false);
  effect.setParams(params);

  constexpr int kWidth = 6;
  constexpr int kHeight = 6;
  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(kWidth * kHeight * 4), 0);
  fillPattern(pixels, kWidth, kHeight);
  std::vector<std::uint8_t> history = pixels;

  auto ctx = makeContext(pixels, kWidth, kHeight);
  ASSERT_TRUE(effect.render(ctx));

  const std::array<std::uint8_t, 4> pivotColor = readPixel(pixels, kWidth, kWidth / 2, kHeight / 2);

  // Force a zoom around the top-left corner using the stored history.
  std::fill(pixels.begin(), pixels.end(), 0);
  avs::core::ParamBlock zoomParams;
  zoomParams.setInt("zoom_scale", 62);
  zoomParams.setString("anchor", "top_left");
  zoomParams.setBool("subpixel", false);
  effect.setParams(zoomParams);
  ASSERT_TRUE(effect.render(ctx));

  const auto topLeft = readPixel(pixels, kWidth, 0, 0);
  const auto originalTopLeft = readPixel(history, kWidth, 0, 0);
  EXPECT_EQ(topLeft, originalTopLeft);

  const auto center = readPixel(pixels, kWidth, kWidth / 2, kHeight / 2);
  EXPECT_NE(center, pivotColor);
}

TEST(RotoBlitter, BlendsWithCurrentFrameWhenEnabled) {
  avs::effects::trans::RotoBlitter effect;
  avs::core::ParamBlock params;
  params.setInt("rot_dir", 32);
  params.setInt("zoom_scale", 31);
  params.setBool("subpixel", false);
  params.setBool("blend", true);
  effect.setParams(params);

  constexpr int kWidth = 4;
  constexpr int kHeight = 4;
  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(kWidth * kHeight * 4), 0);
  fillPattern(pixels, kWidth, kHeight);
  auto ctx = makeContext(pixels, kWidth, kHeight);
  ASSERT_TRUE(effect.render(ctx));
  std::vector<std::uint8_t> previous = pixels;

  std::vector<std::uint8_t> alternate(static_cast<std::size_t>(kWidth * kHeight * 4), 0);
  for (std::size_t i = 0; i < alternate.size(); i += 4) {
    alternate[i + 0] = 255;
    alternate[i + 1] = 0;
    alternate[i + 2] = 0;
    alternate[i + 3] = 255;
  }
  pixels = alternate;
  ctx.framebuffer = {pixels.data(), pixels.size()};
  ASSERT_TRUE(effect.render(ctx));

  const auto blended = readPixel(pixels, kWidth, 0, 0);
  const auto historySample = readPixel(previous, kWidth, 0, 0);
  EXPECT_EQ(blended[0], static_cast<std::uint8_t>((historySample[0] + 255) / 2));
  EXPECT_EQ(blended[1], static_cast<std::uint8_t>((historySample[1] + 0) / 2));
  EXPECT_EQ(blended[2], static_cast<std::uint8_t>((historySample[2] + 0) / 2));
  EXPECT_EQ(blended[3], static_cast<std::uint8_t>((historySample[3] + 255) / 2));
}

