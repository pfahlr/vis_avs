#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <vector>

#include "avs/core/ParamBlock.hpp"
#include "avs/core/RenderContext.hpp"
#include "effects/render/effect_timescope.h"

namespace {

std::array<std::uint8_t, 4> pixelAt(const std::vector<std::uint8_t>& buffer, int width, int x,
                                    int y) {
  const std::size_t index = (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                             static_cast<std::size_t>(x)) *
                            4u;
  return {buffer[index + 0], buffer[index + 1], buffer[index + 2], buffer[index + 3]};
}

}  // namespace

TEST(RenderTimescopeTest, WritesSpectrumColumn) {
  avs::effects::render::Timescope effect;
  avs::core::ParamBlock params;
  params.setInt("blend", 0);
  params.setInt("nbands", 16);
  params.setInt("color", 0x00FF00);
  effect.setParams(params);

  const int width = 5;
  const int height = 4;
  std::vector<std::uint8_t> framebuffer(static_cast<std::size_t>(width * height * 4), 0u);

  std::array<float, 16> spectrum{};
  for (std::size_t i = 0; i < spectrum.size(); ++i) {
    spectrum[i] = static_cast<float>(i);
  }

  avs::core::RenderContext context{};
  context.width = width;
  context.height = height;
  context.framebuffer = {framebuffer.data(), framebuffer.size()};
  context.audioSpectrum = {spectrum.data(), spectrum.size()};

  ASSERT_TRUE(effect.render(context));

  auto top = pixelAt(framebuffer, width, 0, 0);
  auto mid1 = pixelAt(framebuffer, width, 0, 1);
  auto mid2 = pixelAt(framebuffer, width, 0, 2);
  auto bottom = pixelAt(framebuffer, width, 0, 3);
  auto untouched = pixelAt(framebuffer, width, 1, 0);

  EXPECT_EQ(top[0], 0u);
  EXPECT_EQ(top[1], 0u);
  EXPECT_EQ(top[2], 0u);
  EXPECT_EQ(top[3], 255u);

  EXPECT_EQ(mid1[0], 0u);
  EXPECT_EQ(mid1[1], 68u);
  EXPECT_EQ(mid1[2], 0u);
  EXPECT_EQ(mid1[3], 255u);

  EXPECT_EQ(mid2[0], 0u);
  EXPECT_EQ(mid2[1], 136u);
  EXPECT_EQ(mid2[2], 0u);
  EXPECT_EQ(mid2[3], 255u);

  EXPECT_EQ(bottom[0], 0u);
  EXPECT_EQ(bottom[1], 204u);
  EXPECT_EQ(bottom[2], 0u);
  EXPECT_EQ(bottom[3], 255u);

  EXPECT_EQ(untouched[0], 0u);
  EXPECT_EQ(untouched[1], 0u);
  EXPECT_EQ(untouched[2], 0u);
  EXPECT_EQ(untouched[3], 0u);
}
