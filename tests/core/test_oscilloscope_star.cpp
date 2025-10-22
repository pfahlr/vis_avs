#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include "audio/analyzer.h"
#include "avs/core/RenderContext.hpp"
#include "effects/render/effect_oscilloscope_star.h"

namespace {

constexpr int kWidth = 96;
constexpr int kHeight = 96;

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

void seedWaveform(avs::audio::Analysis& analysis) {
  analysis.spectrum.fill(0.0f);
  for (std::size_t i = 0; i < analysis.waveform.size(); ++i) {
    const double t = static_cast<double>(i) / static_cast<double>(analysis.waveform.size());
    analysis.waveform[i] = static_cast<float>(std::sin(t * 6.283185307179586));
  }
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

double averageLitX(const std::vector<std::uint8_t>& pixels) {
  double sum = 0.0;
  int count = 0;
  for (int y = 0; y < kHeight; ++y) {
    for (int x = 0; x < kWidth; ++x) {
      const std::size_t index = (static_cast<std::size_t>(y) * static_cast<std::size_t>(kWidth) +
                                 static_cast<std::size_t>(x)) *
                                4u;
      if (index + 2u >= pixels.size()) {
        continue;
      }
      if (pixels[index] != 0 || pixels[index + 1u] != 0 || pixels[index + 2u] != 0) {
        sum += static_cast<double>(x);
        ++count;
      }
    }
  }
  if (count == 0) {
    return static_cast<double>(kWidth) * 0.5;
  }
  return sum / static_cast<double>(count);
}

}  // namespace

TEST(OscilloscopeStarEffectTest, DrawsStarWithWaveform) {
  avs::effects::render::OscilloscopeStar effect;
  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(kWidth) * static_cast<std::size_t>(kHeight) * 4u, 0u);
  avs::audio::Analysis analysis{};
  seedWaveform(analysis);
  auto context = makeContext(pixels, analysis);

  ASSERT_TRUE(effect.render(context));

  EXPECT_GT(countLitPixels(pixels), 0);
}

TEST(OscilloscopeStarEffectTest, HonorsCustomPalette) {
  avs::effects::render::OscilloscopeStar effect;
  avs::core::ParamBlock params;
  params.setString("colors", "#FF0000");
  effect.setParams(params);

  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(kWidth) * static_cast<std::size_t>(kHeight) * 4u, 0u);
  avs::audio::Analysis analysis{};
  seedWaveform(analysis);
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

TEST(OscilloscopeStarEffectTest, ParsesCommaSeparatedPaletteTokens) {
  avs::effects::render::OscilloscopeStar effect;
  avs::core::ParamBlock params;
  params.setString("colors", "#FF0000,#00FF00");
  effect.setParams(params);

  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(kWidth) * static_cast<std::size_t>(kHeight) * 4u, 0u);
  avs::audio::Analysis analysis{};
  bool sawGreenDominant = false;

  for (int frame = 0; frame < 256 && !sawGreenDominant; ++frame) {
    std::fill(pixels.begin(), pixels.end(), 0u);
    seedWaveform(analysis);
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

TEST(OscilloscopeStarEffectTest, ZeroSizeProducesNoOutput) {
  avs::effects::render::OscilloscopeStar effect;
  avs::core::ParamBlock params;
  params.setInt("size", 0);
  effect.setParams(params);

  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(kWidth) * static_cast<std::size_t>(kHeight) * 4u, 0u);
  avs::audio::Analysis analysis{};
  seedWaveform(analysis);
  auto context = makeContext(pixels, analysis);

  ASSERT_TRUE(effect.render(context));

  EXPECT_EQ(countLitPixels(pixels), 0);
}

TEST(OscilloscopeStarEffectTest, EffectBitsControlAnchorPosition) {
  avs::effects::render::OscilloscopeStar effect;
  avs::core::ParamBlock params;
  params.setInt("effect", (2 << 2) | (0 << 4));  // mid channel, left anchor
  effect.setParams(params);

  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(kWidth) * static_cast<std::size_t>(kHeight) * 4u, 0u);
  avs::audio::Analysis analysis{};
  seedWaveform(analysis);
  auto context = makeContext(pixels, analysis);

  ASSERT_TRUE(effect.render(context));

  EXPECT_LT(averageLitX(pixels), static_cast<double>(kWidth) * 0.5);
}
