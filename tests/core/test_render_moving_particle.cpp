#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <vector>

#include "avs/core/ParamBlock.hpp"
#include "avs/core/RenderContext.hpp"
#include "effects/render/effect_moving_particle.h"

namespace {

avs::core::RenderContext makeContext(int width, int height, std::vector<std::uint8_t>& buffer) {
  avs::core::RenderContext context{};
  context.width = width;
  context.height = height;
  context.deltaSeconds = 1.0 / 60.0;
  context.frameIndex = 0;
  context.framebuffer = {buffer.data(), buffer.size()};
  context.rng.reseed(context.frameIndex);
  return context;
}

int countPixelsWithColor(const std::vector<std::uint8_t>& buffer, int width, int height,
                         std::uint8_t r, std::uint8_t g, std::uint8_t b) {
  int count = 0;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const std::size_t offset =
          (static_cast<std::size_t>(y) * width + static_cast<std::size_t>(x)) * 4u;
      if (buffer[offset + 0] == r && buffer[offset + 1] == g && buffer[offset + 2] == b) {
        ++count;
      }
    }
  }
  return count;
}

}  // namespace

TEST(RenderMovingParticleEffect, DisabledSkipsRendering) {
  Effect_RenderMovingParticle effect;
  avs::core::ParamBlock params;
  params.setBool("enabled", false);
  effect.setParams(params);

  constexpr int kWidth = 32;
  constexpr int kHeight = 24;
  std::vector<std::uint8_t> buffer(static_cast<std::size_t>(kWidth) * kHeight * 4u, 17u);
  auto context = makeContext(kWidth, kHeight, buffer);

  ASSERT_TRUE(effect.render(context));
  EXPECT_TRUE(
      std::all_of(buffer.begin(), buffer.end(), [](std::uint8_t value) { return value == 17u; }));
}

TEST(RenderMovingParticleEffect, DrawsSinglePixelWhenRadiusIsOne) {
  Effect_RenderMovingParticle effect;
  avs::core::ParamBlock params;
  params.setInt("size", 1);
  params.setInt("size2", 1);
  params.setString("blend_mode", "replace");
  effect.setParams(params);

  constexpr int kWidth = 64;
  constexpr int kHeight = 48;
  std::vector<std::uint8_t> buffer(static_cast<std::size_t>(kWidth) * kHeight * 4u, 0u);
  auto context = makeContext(kWidth, kHeight, buffer);

  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(countPixelsWithColor(buffer, kWidth, kHeight, 255u, 255u, 255u), 1);
}

TEST(RenderMovingParticleEffect, AverageBlendHalvesPixelTowardsColor) {
  Effect_RenderMovingParticle effect;
  avs::core::ParamBlock params;
  params.setInt("size", 1);
  params.setInt("size2", 1);
  params.setString("blend_mode", "average");
  params.setInt("color", 0x202020);
  effect.setParams(params);

  constexpr int kWidth = 64;
  constexpr int kHeight = 48;
  std::vector<std::uint8_t> buffer(static_cast<std::size_t>(kWidth) * kHeight * 4u, 200u);
  auto context = makeContext(kWidth, kHeight, buffer);

  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(countPixelsWithColor(buffer, kWidth, kHeight, 116u, 116u, 116u), 1);
}
