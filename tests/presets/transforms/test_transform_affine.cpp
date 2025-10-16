#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <string>
#include <vector>

#include "avs/core/DeterministicRng.hpp"
#include "avs/core/ParamBlock.hpp"
#include "avs/core/RenderContext.hpp"
#include "effects/transform_affine.h"

namespace {

using avs::core::ParamBlock;
using avs::core::RenderContext;
using avs::core::TransformAffineEffect;

RenderContext makeContext(int width, int height, std::vector<std::uint8_t>& pixels) {
  RenderContext ctx;
  ctx.width = width;
  ctx.height = height;
  ctx.frameIndex = 0;
  ctx.deltaSeconds = 1.0 / 60.0;
  ctx.framebuffer = {pixels.data(), pixels.size()};
  ctx.gating.active = true;
  ctx.testMode = true;
  return ctx;
}

TEST(TransformAffineEffectTest, RotatesAroundAnchors) {
  constexpr int kSize = 64;
  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(kSize) * kSize * 4u, 0);
  TransformAffineEffect effect;
  ParamBlock params;
  params.setFloat("rotation_deg", 90.0f);
  params.setFloat("scale_x", 1.0f);
  params.setFloat("scale_y", 1.0f);
  params.setBool("crosshair", true);
  effect.setParams(params);

  struct Case {
    std::string anchor;
    TransformAffineEffect::Vec2 expectedAnchor;
  };

  const std::array<Case, 5> cases = {Case{"center", {31.5f, 31.5f}},
                                     Case{"top_left", {0.0f, 0.0f}},
                                     Case{"top_right", {63.0f, 0.0f}},
                                     Case{"bottom_left", {0.0f, 63.0f}},
                                     Case{"bottom_right", {63.0f, 63.0f}}};

  for (const auto& c : cases) {
    std::vector<std::uint8_t> local = pixels;
    RenderContext ctx = makeContext(kSize, kSize, local);
    ctx.rng = avs::core::DeterministicRng();
    ctx.rng.reseed(ctx.frameIndex);
    ParamBlock p = params;
    p.setString("anchor", c.anchor);
    effect.setParams(p);
    ASSERT_TRUE(effect.render(ctx));

    auto anchor = effect.lastAnchor();
    EXPECT_NEAR(anchor.x, c.expectedAnchor.x, 1.0f) << c.anchor;
    EXPECT_NEAR(anchor.y, c.expectedAnchor.y, 1.0f) << c.anchor;

    const auto& triangle = effect.lastTriangle();
    float expectedSize = static_cast<float>(kSize) * 0.25f;
    EXPECT_NEAR(triangle[0].x, anchor.x + expectedSize, 1.0f) << c.anchor;
    EXPECT_NEAR(triangle[0].y, anchor.y, 1.0f) << c.anchor;

    int ax = std::clamp(static_cast<int>(std::round(anchor.x)), 0, kSize - 1);
    int ay = std::clamp(static_cast<int>(std::round(anchor.y)), 0, kSize - 1);
    std::size_t idx = (static_cast<std::size_t>(ay) * kSize + static_cast<std::size_t>(ax)) * 4u;
    ASSERT_LT(idx + 2, local.size());
    EXPECT_EQ(local[idx + 1], 255u) << "Crosshair missing on Y channel for " << c.anchor;
    EXPECT_EQ(local[idx + 0], 60u) << "Crosshair missing on X channel for " << c.anchor;
  }
}

TEST(TransformAffineEffectTest, RandomStarfieldDeterministicWithSeed) {
  constexpr int kSize = 48;
  ASSERT_EQ(setenv("VIS_AVS_SEED", "777", 1), 0);

  TransformAffineEffect effectA;
  ParamBlock params;
  params.setBool("draw_shape", false);
  params.setInt("star_count", 64);
  effectA.setParams(params);

  std::vector<std::uint8_t> pixelsA(static_cast<std::size_t>(kSize) * kSize * 4u, 0);
  RenderContext ctxA;
  ctxA.width = kSize;
  ctxA.height = kSize;
  ctxA.frameIndex = 4;
  ctxA.framebuffer = {pixelsA.data(), pixelsA.size()};
  ctxA.gating.active = true;
  ctxA.rng = avs::core::DeterministicRng();
  ctxA.rng.reseed(ctxA.frameIndex);
  ASSERT_TRUE(effectA.render(ctxA));

  TransformAffineEffect effectB;
  effectB.setParams(params);
  std::vector<std::uint8_t> pixelsB(static_cast<std::size_t>(kSize) * kSize * 4u, 0);
  RenderContext ctxB;
  ctxB.width = kSize;
  ctxB.height = kSize;
  ctxB.frameIndex = 4;
  ctxB.framebuffer = {pixelsB.data(), pixelsB.size()};
  ctxB.gating.active = true;
  ctxB.rng = avs::core::DeterministicRng();
  ctxB.rng.reseed(ctxB.frameIndex);
  ASSERT_TRUE(effectB.render(ctxB));

  EXPECT_EQ(pixelsA, pixelsB);
}

}  // namespace
