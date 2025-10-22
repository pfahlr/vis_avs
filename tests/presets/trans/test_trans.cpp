#include <gtest/gtest.h>

#include <array>
#include <cstdint>

#include <avs/effects/trans/effect_color_clip.h>

namespace {

avs::core::RenderContext makeContext(std::uint8_t* data, std::size_t size, int width, int height) {
  avs::core::RenderContext context{};
  context.width = width;
  context.height = height;
  context.framebuffer = {data, size};
  return context;
}

}  // namespace

TEST(ColorClipEffect, ReplacesPixelsBelowThreshold) {
  avs::effects::trans::ColorClip effect;
  avs::core::ParamBlock params;
  const std::uint8_t clipR = 0x30;
  const std::uint8_t clipG = 0x60;
  const std::uint8_t clipB = 0x90;
  params.setInt("color", static_cast<int>(clipR) | (static_cast<int>(clipG) << 8) |
                             (static_cast<int>(clipB) << 16));
  effect.setParams(params);

  std::array<std::uint8_t, 8> pixels{
      0x10u, 0x20u, 0x30u, 0xFFu,  // below thresholds -> replace
      0x40u, 0x80u, 0xA0u, 0xFFu   // exceeds thresholds -> unchanged
  };

  auto context = makeContext(pixels.data(), pixels.size(), 2, 1);
  ASSERT_TRUE(effect.render(context));

  EXPECT_EQ(pixels[0], clipR);
  EXPECT_EQ(pixels[1], clipG);
  EXPECT_EQ(pixels[2], clipB);
  EXPECT_EQ(pixels[3], 0xFFu);

  EXPECT_EQ(pixels[4], 0x40u);
  EXPECT_EQ(pixels[5], 0x80u);
  EXPECT_EQ(pixels[6], 0xA0u);
  EXPECT_EQ(pixels[7], 0xFFu);
}

TEST(ColorClipEffect, HonorsEnabledFlag) {
  avs::effects::trans::ColorClip effect;
  avs::core::ParamBlock params;
  params.setBool("enabled", false);
  params.setInt("color", 0x00010203);
  effect.setParams(params);

  std::array<std::uint8_t, 4> pixel{0x00u, 0x00u, 0x00u, 0xFFu};
  auto context = makeContext(pixel.data(), pixel.size(), 1, 1);
  ASSERT_TRUE(effect.render(context));

  EXPECT_EQ(pixel[0], 0x00u);
  EXPECT_EQ(pixel[1], 0x00u);
  EXPECT_EQ(pixel[2], 0x00u);
  EXPECT_EQ(pixel[3], 0xFFu);
}

