#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <vector>

#include <avs/audio/analyzer.h>
#include <avs/core/ParamBlock.hpp>
#include <avs/core/RenderContext.hpp>
#include <avs/effects/render/effect_dot_fountain.h>

namespace {

constexpr int kWidth = 96;
constexpr int kHeight = 72;
constexpr int kDivisionCount = 30;

avs::core::RenderContext makeContext(std::vector<std::uint8_t>& framebuffer) {
  avs::core::RenderContext context{};
  context.width = kWidth;
  context.height = kHeight;
  context.framebuffer = {framebuffer.data(), framebuffer.size()};
  context.deltaSeconds = 1.0 / 60.0;
  return context;
}

}  // namespace

TEST(RenderDotFountainEffectTest, ProducesPixelsWhenSpectrumIsPresent) {
  Effect_RenderDotFountain effect;
  avs::core::ParamBlock params;
  params.setInt("rotvel", 12);
  effect.setParams(params);

  std::vector<std::uint8_t> framebuffer(static_cast<std::size_t>(kWidth) *
                                        static_cast<std::size_t>(kHeight) * 4u, 0u);
  std::vector<float> spectrum(avs::audio::Analysis::kSpectrumSize, 0.0f);
  std::fill(spectrum.begin(), spectrum.begin() + kDivisionCount, 1.0f);

  auto context = makeContext(framebuffer);
  context.audioSpectrum = {spectrum.data(), spectrum.size()};
  context.audioBeat = true;

  ASSERT_TRUE(effect.render(context));
  const bool hasColor = std::any_of(framebuffer.begin(), framebuffer.end(),
                                    [](std::uint8_t value) { return value != 0; });
  EXPECT_TRUE(hasColor);
}

TEST(RenderDotFountainEffectTest, RespectsPaletteUpdates) {
  Effect_RenderDotFountain effect;
  avs::core::ParamBlock params;
  params.setInt("color0", 0xFF0000);
  params.setInt("color1", 0xFF0000);
  params.setInt("color2", 0xFF0000);
  params.setInt("color3", 0xFF0000);
  params.setInt("color4", 0xFF0000);
  effect.setParams(params);

  std::vector<std::uint8_t> framebuffer(static_cast<std::size_t>(kWidth) *
                                        static_cast<std::size_t>(kHeight) * 4u, 0u);
  std::vector<float> spectrum(avs::audio::Analysis::kSpectrumSize, 1.0f);

  auto context = makeContext(framebuffer);
  context.audioSpectrum = {spectrum.data(), spectrum.size()};
  context.audioBeat = true;

  ASSERT_TRUE(effect.render(context));
  bool observedRed = false;
  for (std::size_t i = 0; i < framebuffer.size(); i += 4) {
    const std::uint8_t r = framebuffer[i + 0];
    const std::uint8_t g = framebuffer[i + 1];
    const std::uint8_t b = framebuffer[i + 2];
    if (r > 0) {
      EXPECT_EQ(g, 0u);
      EXPECT_EQ(b, 0u);
      observedRed = true;
      break;
    }
  }
  EXPECT_TRUE(observedRed);
}

TEST(RenderDotFountainEffectTest, HandlesMissingSpectrumGracefully) {
  Effect_RenderDotFountain effect;
  std::vector<std::uint8_t> framebuffer(static_cast<std::size_t>(kWidth) *
                                        static_cast<std::size_t>(kHeight) * 4u, 0u);
  auto context = makeContext(framebuffer);

  EXPECT_TRUE(effect.render(context));
}
