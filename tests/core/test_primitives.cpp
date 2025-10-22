#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <vector>

#include <avs/core/ParamBlock.hpp>
#include <avs/core/RenderContext.hpp>
#include <avs/effects/Primitives.hpp>

namespace {

struct TestContext {
  avs::core::RenderContext ctx;
  std::vector<std::uint8_t> buffer;
};

TestContext makeContext(int width, int height) {
  TestContext tc;
  tc.buffer.resize(static_cast<std::size_t>(width) * height * 4u, 0);
  tc.ctx.width = width;
  tc.ctx.height = height;
  tc.ctx.framebuffer.data = tc.buffer.data();
  tc.ctx.framebuffer.size = tc.buffer.size();
  tc.ctx.frameIndex = 0;
  tc.ctx.deltaSeconds = 0.0;
  return tc;
}

std::size_t pixelIndex(int x, int y, int width) {
  return (static_cast<std::size_t>(y) * width + x) * 4u;
}

}  // namespace

TEST(PrimitiveEffects, SolidFill) {
  auto tc = makeContext(8, 8);
  avs::core::ParamBlock params;
  params.setInt("x1", 1);
  params.setInt("y1", 1);
  params.setInt("x2", 3);
  params.setInt("y2", 3);
  params.setInt("color", 0x00FF00);
  avs::effects::PrimitiveSolid solid;
  solid.setParams(params);
  ASSERT_TRUE(solid.render(tc.ctx));
  const std::size_t idx = pixelIndex(2, 2, tc.ctx.width);
  EXPECT_EQ(tc.buffer[idx + 1], 255);  // green channel
  EXPECT_EQ(tc.buffer[idx], 0);
  EXPECT_EQ(tc.buffer[idx + 2], 0);
}

TEST(PrimitiveEffects, DotDrawsCircle) {
  auto tc = makeContext(8, 8);
  avs::core::ParamBlock params;
  params.setString("points", "4,4");
  params.setInt("radius", 1);
  params.setInt("color", 0xFF0000);
  avs::effects::PrimitiveDots dots;
  dots.setParams(params);
  ASSERT_TRUE(dots.render(tc.ctx));
  const std::size_t center = pixelIndex(4, 4, tc.ctx.width);
  EXPECT_EQ(tc.buffer[center], 255);  // red channel
}

TEST(PrimitiveEffects, LineSegmentsConnectPoints) {
  auto tc = makeContext(10, 10);
  avs::core::ParamBlock params;
  params.setString("points", "1,1 6,6");
  params.setInt("color", 0x0000FF);
  avs::effects::PrimitiveLines lines;
  lines.setParams(params);
  ASSERT_TRUE(lines.render(tc.ctx));
  const std::size_t idx = pixelIndex(4, 4, tc.ctx.width);
  EXPECT_EQ(tc.buffer[idx + 2], 255);  // blue channel
}

TEST(PrimitiveEffects, TriangleFillAndOutline) {
  auto tc = makeContext(12, 12);
  avs::core::ParamBlock params;
  params.setString("triangles", "2,2 9,2 4,9");
  params.setInt("color", 0x00FFFF);
  params.setInt("outlinecolor", 0xFF0000);
  params.setInt("outlinealpha", 255);
  params.setInt("outlinesize", 1);
  avs::effects::PrimitiveTriangles tri;
  tri.setParams(params);
  ASSERT_TRUE(tri.render(tc.ctx));
  const std::size_t inside = pixelIndex(4, 4, tc.ctx.width);
  EXPECT_GT(tc.buffer[inside + 1], 0);  // filled with cyan => green > 0
  const std::size_t edge = pixelIndex(2, 2, tc.ctx.width);
  EXPECT_GT(tc.buffer[edge], 0);  // outline touches the top vertex (red channel)
}

TEST(PrimitiveEffects, RoundedRectangleOutline) {
  auto tc = makeContext(12, 12);
  avs::core::ParamBlock params;
  params.setInt("x", 2);
  params.setInt("y", 2);
  params.setInt("width", 6);
  params.setInt("height", 6);
  params.setInt("radius", 2);
  params.setInt("color", 0xFFFFFF);
  params.setInt("outlinecolor", 0x0000FF);
  params.setInt("outlinesize", 1);
  params.setBool("filled", true);
  avs::effects::PrimitiveRoundedRect rect;
  rect.setParams(params);
  ASSERT_TRUE(rect.render(tc.ctx));
  const std::size_t inside = pixelIndex(4, 4, tc.ctx.width);
  EXPECT_EQ(tc.buffer[inside], 255);
  EXPECT_EQ(tc.buffer[inside + 1], 255);
  EXPECT_EQ(tc.buffer[inside + 2], 255);
}

TEST(TextEffect, AntialiasToggleProducesCoverage) {
  avs::core::ParamBlock params;
  params.setString("text", "A");
  params.setInt("x", 4);
  params.setInt("y", 4);
  params.setInt("size", 12);
  params.setInt("color", 0x0000FF);

  auto ctxNoAA = makeContext(32, 32);
  avs::effects::Text textNoAA;
  textNoAA.setParams(params);
  ASSERT_TRUE(textNoAA.render(ctxNoAA.ctx));

  auto countPartial = [&](const std::vector<std::uint8_t>& buf) {
    int partial = 0;
    for (std::size_t i = 0; i < buf.size(); i += 4) {
      const std::uint8_t blue = buf[i + 2];
      if (blue > 0 && blue < 255) {
        ++partial;
      }
    }
    return partial;
  };

  int partialNoAA = countPartial(ctxNoAA.buffer);

  auto ctxAA = makeContext(32, 32);
  params.setBool("antialias", true);
  avs::effects::Text textAA;
  textAA.setParams(params);
  ASSERT_TRUE(textAA.render(ctxAA.ctx));
  int partialAA = countPartial(ctxAA.buffer);

  EXPECT_EQ(partialNoAA, 0);
  EXPECT_GT(partialAA, 0);
}

TEST(TextEffect, OutlineAndShadowLayering) {
  auto tc = makeContext(48, 32);
  avs::core::ParamBlock params;
  params.setString("text", "TEXT");
  params.setInt("x", 16);
  params.setInt("y", 16);
  params.setInt("size", 12);
  params.setInt("color", 0x0000FF);
  params.setInt("outlinecolor", 0x00FF00);
  params.setInt("outlinesize", 1);
  params.setInt("shadowcolor", 0xFF0000);
  params.setInt("shadowalpha", 200);
  params.setInt("shadowoffsetx", 2);
  params.setInt("shadowoffsety", 2);
  params.setInt("shadowblur", 1);
  params.setBool("shadow", true);
  avs::effects::Text effect;
  effect.setParams(params);
  ASSERT_TRUE(effect.render(tc.ctx));

  bool hasBlue = false;
  bool hasGreen = false;
  bool hasRed = false;
  for (std::size_t i = 0; i < tc.buffer.size(); i += 4) {
    hasBlue = hasBlue || tc.buffer[i + 2] > 0;
    hasGreen = hasGreen || tc.buffer[i + 1] > 0;
    hasRed = hasRed || tc.buffer[i] > 0;
  }
  EXPECT_TRUE(hasBlue);
  EXPECT_TRUE(hasGreen);
  EXPECT_TRUE(hasRed);
}

