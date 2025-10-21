#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "avs/core/EffectRegistry.hpp"
#include "avs/core/ParamBlock.hpp"
#include "avs/core/Pipeline.hpp"
#include "avs/core/RenderContext.hpp"
#include "avs/effects/RegisterEffects.hpp"
#include "avs/runtime/GlobalState.hpp"

namespace {

std::size_t pixelIndex(int x, int y, int width) {
  return (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)) * 4u;
}

}  // namespace

TEST(RenderModeEffect, AppliesAdditiveBlendToLines) {
  avs::core::EffectRegistry registry;
  avs::effects::registerCoreEffects(registry);
  avs::core::Pipeline pipeline(registry);

  avs::core::ParamBlock renderModeParams;
  renderModeParams.setString("mode", "additive");
  pipeline.add("misc / set render mode", renderModeParams);

  avs::core::ParamBlock lineParams;
  lineParams.setString("points", "0,0 3,0");
  lineParams.setInt("color", 0x400000);
  lineParams.setInt("alpha", 255);
  pipeline.add("line", lineParams);

  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(4 * 1 * 4), 0);
  for (std::size_t i = 0; i < pixels.size(); i += 4) {
    pixels[i + 0] = 10;   // red
    pixels[i + 1] = 0;    // green
    pixels[i + 2] = 0;    // blue
    pixels[i + 3] = 255;  // alpha
  }

  avs::runtime::GlobalState globals;
  avs::core::RenderContext context;
  context.width = 4;
  context.height = 1;
  context.framebuffer = {pixels.data(), pixels.size()};
  context.globals = &globals;
  context.frameIndex = 0;
  context.deltaSeconds = 1.0 / 60.0;

  ASSERT_TRUE(pipeline.render(context));

  const auto idx = pixelIndex(0, 0, context.width);
  EXPECT_EQ(pixels[idx + 0], 74);  // 10 + 64 = 74
  EXPECT_EQ(pixels[idx + 1], 0);
  EXPECT_EQ(pixels[idx + 2], 0);
  EXPECT_EQ(pixels[idx + 3], 255);
}

TEST(RenderModeEffect, UsesOverrideLineWidthWhenNotExplicit) {
  avs::core::EffectRegistry registry;
  avs::effects::registerCoreEffects(registry);
  avs::core::Pipeline pipeline(registry);

  avs::core::ParamBlock renderModeParams;
  renderModeParams.setInt("line_width", 3);
  pipeline.add("misc / set render mode", renderModeParams);

  avs::core::ParamBlock lineParams;
  lineParams.setString("points", "0,2 4,2");
  lineParams.setInt("color", 0xFF0000);
  lineParams.setInt("alpha", 255);
  pipeline.add("line", lineParams);

  const int width = 5;
  const int height = 5;
  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width * height * 4), 0);

  avs::runtime::GlobalState globals;
  avs::core::RenderContext context;
  context.width = width;
  context.height = height;
  context.framebuffer = {pixels.data(), pixels.size()};
  context.globals = &globals;
  context.frameIndex = 0;
  context.deltaSeconds = 1.0 / 60.0;

  ASSERT_TRUE(pipeline.render(context));

  const auto aboveIdx = pixelIndex(2, 1, width);
  const auto midIdx = pixelIndex(2, 2, width);
  EXPECT_GT(pixels[midIdx + 0], 0);
  EXPECT_GT(pixels[aboveIdx + 0], 0);  // thickness expanded above the centre line
}

