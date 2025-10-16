#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <vector>

#include "avs/core/DeterministicRng.hpp"
#include "avs/core/ParamBlock.hpp"
#include "avs/core/RenderContext.hpp"
#include "effects/gating.h"
#include "effects/transform_affine.h"

namespace {

using avs::core::BeatGatingEffect;
using avs::core::ParamBlock;
using avs::core::RenderContext;
using avs::core::TransformAffineEffect;

RenderContext makeContext(int width, int height, std::vector<std::uint8_t>& pixels) {
  RenderContext ctx;
  ctx.width = width;
  ctx.height = height;
  ctx.framebuffer = {pixels.data(), pixels.size()};
  ctx.deltaSeconds = 1.0 / 60.0;
  ctx.gating.active = true;
  return ctx;
}

TEST(BeatGatingEffectTest, ActivatesOnlyOnBeat) {
  constexpr int kSize = 16;
  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(kSize) * kSize * 4u, 0);
  RenderContext ctx = makeContext(kSize, kSize, pixels);

  BeatGatingEffect gating;
  ParamBlock params;
  params.setBool("onbeat", true);
  params.setBool("stick", false);
  params.setInt("log_height", 4);
  gating.setParams(params);

  ctx.frameIndex = 0;
  ctx.beat = {false, true};
  ctx.rng = avs::core::DeterministicRng();
  ctx.rng.reseed(ctx.frameIndex);
  ASSERT_TRUE(gating.render(ctx));
  EXPECT_FALSE(ctx.gating.active);
  EXPECT_FALSE(ctx.gating.triggered);

  ctx.frameIndex = 1;
  ctx.beat = {true, true};
  ctx.rng.reseed(ctx.frameIndex);
  ASSERT_TRUE(gating.render(ctx));
  EXPECT_TRUE(ctx.gating.active);
  EXPECT_TRUE(ctx.gating.triggered);
  EXPECT_EQ(ctx.gating.flag, 2u);

  ctx.frameIndex = 2;
  ctx.beat = {false, true};
  ctx.rng.reseed(ctx.frameIndex);
  ASSERT_TRUE(gating.render(ctx));
  EXPECT_FALSE(ctx.gating.active);
  EXPECT_FALSE(ctx.gating.triggered);
}

TEST(BeatGatingEffectTest, StickyLatchPersistsAcrossFrames) {
  constexpr int kSize = 8;
  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(kSize) * kSize * 4u, 0);
  RenderContext ctx = makeContext(kSize, kSize, pixels);

  BeatGatingEffect gating;
  ParamBlock params;
  params.setBool("onbeat", true);
  params.setBool("stick", true);
  params.setInt("log_height", 0);
  gating.setParams(params);

  ctx.rng = avs::core::DeterministicRng();

  ctx.frameIndex = 0;
  ctx.beat = {false, true};
  ctx.rng.reseed(ctx.frameIndex);
  ASSERT_TRUE(gating.render(ctx));
  EXPECT_FALSE(ctx.gating.active);

  ctx.frameIndex = 1;
  ctx.beat = {true, true};
  ctx.rng.reseed(ctx.frameIndex);
  ASSERT_TRUE(gating.render(ctx));
  EXPECT_TRUE(ctx.gating.active);
  EXPECT_TRUE(ctx.gating.latched);

  ctx.frameIndex = 2;
  ctx.beat = {false, true};
  ctx.rng.reseed(ctx.frameIndex);
  ASSERT_TRUE(gating.render(ctx));
  EXPECT_TRUE(ctx.gating.active);
  EXPECT_TRUE(ctx.gating.latched);
}

TEST(BeatGatingEffectTest, OnlyStickyBlocksUncertainBeats) {
  constexpr int kSize = 32;
  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(kSize) * kSize * 4u, 0);
  RenderContext ctx = makeContext(kSize, kSize, pixels);

  BeatGatingEffect gating;
  ParamBlock params;
  params.setBool("onbeat", true);
  params.setBool("only_sticky", true);
  params.setInt("log_height", 4);
  gating.setParams(params);

  ctx.rng = avs::core::DeterministicRng();

  ctx.frameIndex = 0;
  ctx.beat = {true, false};
  ctx.rng.reseed(ctx.frameIndex);
  ASSERT_TRUE(gating.render(ctx));
  EXPECT_FALSE(ctx.gating.active);
  EXPECT_EQ(ctx.gating.flag, 3u);

  int x = kSize - 1;
  std::size_t idx = static_cast<std::size_t>(x) * 4u;
  EXPECT_EQ(pixels[idx + 0], 30u);
  EXPECT_EQ(pixels[idx + 1], 110u);
  EXPECT_EQ(pixels[idx + 2], 210u);
}

TEST(BeatGatingEffectTest, ControlsTransformRendering) {
  constexpr int kSize = 48;
  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(kSize) * kSize * 4u, 0);
  RenderContext ctx = makeContext(kSize, kSize, pixels);
  ctx.testMode = false;

  BeatGatingEffect gating;
  ParamBlock gatingParams;
  gatingParams.setBool("onbeat", true);
  gatingParams.setInt("log_height", 0);
  gating.setParams(gatingParams);

  TransformAffineEffect transform;
  ParamBlock transformParams;
  transformParams.setFloat("rotation_deg", 0.0f);
  transformParams.setBool("crosshair", false);
  transform.setParams(transformParams);

  ctx.rng = avs::core::DeterministicRng();

  ctx.frameIndex = 3;
  ctx.beat = {false, true};
  ctx.rng.reseed(ctx.frameIndex);
  ASSERT_TRUE(gating.render(ctx));
  ASSERT_TRUE(transform.render(ctx));
  auto sampleTriangleColor = [&](const TransformAffineEffect& eff) {
    const auto& tri = eff.lastTriangle();
    float cx = (tri[0].x + tri[1].x + tri[2].x) / 3.0f;
    float cy = (tri[0].y + tri[1].y + tri[2].y) / 3.0f;
    int sx = std::clamp(static_cast<int>(std::round(cx)), 0, kSize - 1);
    int sy = std::clamp(static_cast<int>(std::round(cy)), 0, kSize - 1);
    std::size_t idx = (static_cast<std::size_t>(sy) * kSize + static_cast<std::size_t>(sx)) * 4u;
    return static_cast<int>(pixels[idx + 0]) + static_cast<int>(pixels[idx + 1]) +
           static_cast<int>(pixels[idx + 2]);
  };
  EXPECT_EQ(sampleTriangleColor(transform), 0);

  ctx.frameIndex = 4;
  ctx.beat = {true, true};
  ctx.rng.reseed(ctx.frameIndex);
  ASSERT_TRUE(gating.render(ctx));
  ASSERT_TRUE(transform.render(ctx));
  EXPECT_GT(sampleTriangleColor(transform), 0);
}

}  // namespace
