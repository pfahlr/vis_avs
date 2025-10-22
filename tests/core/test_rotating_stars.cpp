#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <vector>

#include "audio/analyzer.h"
#include "avs/core/ParamBlock.hpp"
#include "avs/core/RenderContext.hpp"
#include "effects/render/effect_rotating_stars.h"

namespace {

constexpr int kWidth = 64;
constexpr int kHeight = 64;

avs::core::RenderContext makeContext(std::vector<std::uint8_t>& pixels,
                                     avs::audio::Analysis& analysis) {
  avs::core::RenderContext ctx{};
  ctx.width = kWidth;
  ctx.height = kHeight;
  ctx.framebuffer = {pixels.data(), pixels.size()};
  ctx.audioAnalysis = &analysis;
  ctx.audioSpectrum = {analysis.spectrum.data(), analysis.spectrum.size()};
  return ctx;
}

void seedSpectrum(avs::audio::Analysis& analysis) {
  analysis.spectrum.fill(0.0f);
  analysis.spectrum[3] = 20.0f;
  analysis.spectrum[4] = 80.0f;
}

int countLitPixels(const std::vector<std::uint8_t>& pixels) {
  int lit = 0;
  for (std::size_t i = 0; i + 3 < pixels.size(); i += 4) {
    if (pixels[i] != 0 || pixels[i + 1] != 0 || pixels[i + 2] != 0) {
      ++lit;
    }
  }
  return lit;
}

}  // namespace

TEST(RotatingStarsEffectTest, DrawsStarWithDefaultPalette) {
  avs::effects::render::RotatingStars effect;
  std::vector<std::uint8_t> pixels(
      static_cast<std::size_t>(kWidth) * static_cast<std::size_t>(kHeight) * 4u, 0u);
  avs::audio::Analysis analysis{};
  seedSpectrum(analysis);
  auto context = makeContext(pixels, analysis);

  ASSERT_TRUE(effect.render(context));

  EXPECT_GT(countLitPixels(pixels), 0);
}

TEST(RotatingStarsEffectTest, HonorsCustomColorPalette) {
  avs::effects::render::RotatingStars effect;
  avs::core::ParamBlock params;
  params.setString("colors", "#FF0000");
  effect.setParams(params);

  std::vector<std::uint8_t> pixels(
      static_cast<std::size_t>(kWidth) * static_cast<std::size_t>(kHeight) * 4u, 0u);
  avs::audio::Analysis analysis{};
  seedSpectrum(analysis);
  auto context = makeContext(pixels, analysis);

  ASSERT_TRUE(effect.render(context));

  std::uint8_t maxRed = 0;
  std::size_t maxIndex = 0;
  for (std::size_t i = 0; i + 3 < pixels.size(); i += 4) {
    if (pixels[i] > maxRed) {
      maxRed = pixels[i];
      maxIndex = i;
    }
  }
  EXPECT_GT(maxRed, 0);
  EXPECT_EQ(pixels[maxIndex + 1u], 0u);
  EXPECT_EQ(pixels[maxIndex + 2u], 0u);
}

TEST(RotatingStarsEffectTest, ParsesCommaSeparatedPaletteTokens) {
  avs::effects::render::RotatingStars effect;
  avs::core::ParamBlock params;
  params.setString("colors", "#FF0000,#00FF00");
  effect.setParams(params);

  std::vector<std::uint8_t> pixels(
      static_cast<std::size_t>(kWidth) * static_cast<std::size_t>(kHeight) * 4u, 0u);
  avs::audio::Analysis analysis{};
  bool sawGreenDominant = false;

  for (int frame = 0; frame < 256 && !sawGreenDominant; ++frame) {
    std::fill(pixels.begin(), pixels.end(), 0u);
    seedSpectrum(analysis);
    auto context = makeContext(pixels, analysis);

    ASSERT_TRUE(effect.render(context));

    for (std::size_t i = 0; i + 3 < pixels.size(); i += 4) {
      const std::uint8_t red = pixels[i];
      const std::uint8_t green = pixels[i + 1u];
      const std::uint8_t blue = pixels[i + 2u];
      if (green > red && green > blue && green > 0u) {
        sawGreenDominant = true;
        break;
      }
    }
  }

  EXPECT_TRUE(sawGreenDominant);
}

TEST(RotatingStarsEffectTest, InterpretsIntegerColorParamsAsRgb) {
  avs::effects::render::RotatingStars effect;
  avs::core::ParamBlock params;
  params.setInt("color0", 0xFF0000);
  effect.setParams(params);

  std::vector<std::uint8_t> pixels(
      static_cast<std::size_t>(kWidth) * static_cast<std::size_t>(kHeight) * 4u, 0u);
  avs::audio::Analysis analysis{};
  seedSpectrum(analysis);
  auto context = makeContext(pixels, analysis);

  ASSERT_TRUE(effect.render(context));

  std::uint8_t maxRed = 0;
  std::size_t maxIndex = 0;
  for (std::size_t i = 0; i + 3 < pixels.size(); i += 4) {
    if (pixels[i] > maxRed) {
      maxRed = pixels[i];
      maxIndex = i;
    }
  }
  EXPECT_GT(maxRed, 0);
  EXPECT_EQ(pixels[maxIndex + 1u], 0u);
  EXPECT_EQ(pixels[maxIndex + 2u], 0u);
}
