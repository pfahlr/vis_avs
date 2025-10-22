#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>

#include "avs/core/ParamBlock.hpp"
#include "avs/core/RenderContext.hpp"
#include "effects/render/effect_svp_loader.h"

#if !defined(_WIN32)

TEST(SvpLoaderEffectTest, NoOpOnUnsupportedPlatforms) {
  avs::effects::render::SvpLoader effect;
  avs::core::ParamBlock params;
  params.setString("file", "missing.svp");
  effect.setParams(params);

  std::array<std::uint8_t, 16> pixels{};
  pixels.fill(42u);

  avs::core::RenderContext context{};
  context.width = 2;
  context.height = 2;
  context.framebuffer = {pixels.data(), pixels.size()};

  ASSERT_TRUE(effect.render(context));
  EXPECT_TRUE(
      std::all_of(pixels.begin(), pixels.end(), [](std::uint8_t value) { return value == 42u; }));
}

#endif  // !defined(_WIN32)
