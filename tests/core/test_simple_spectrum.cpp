#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include <avs/audio/analyzer.h>
#include <avs/core/ParamBlock.hpp>
#include <avs/core/RenderContext.hpp>
#include <avs/effects/render/effect_simple_spectrum.h>

namespace {

constexpr int kWidth = 64;
constexpr int kHeight = 48;

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

TEST(SimpleSpectrumEffectTest, DrawsAnalyzerBarsFromSpectrum) {
  avs::effects::render::SimpleSpectrum effect;
  std::vector<std::uint8_t> pixels(
      static_cast<std::size_t>(kWidth) * static_cast<std::size_t>(kHeight) * 4u, 0u);
  avs::audio::Analysis analysis{};
  analysis.spectrum.fill(0.0f);
  analysis.waveform.fill(0.0f);
  analysis.spectrum[5] = 20.0f;
  analysis.spectrum[6] = 40.0f;
  auto context = makeContext(pixels, analysis);

  ASSERT_TRUE(effect.render(context));

  EXPECT_GT(countLitPixels(pixels), 0);
}

TEST(SimpleSpectrumEffectTest, HonorsCustomColorParameter) {
  avs::effects::render::SimpleSpectrum effect;
  avs::core::ParamBlock params;
  params.setInt("num_colors", 1);
  params.setInt("color0", 0x00FF00);
  effect.setParams(params);

  std::vector<std::uint8_t> pixels(
      static_cast<std::size_t>(kWidth) * static_cast<std::size_t>(kHeight) * 4u, 0u);
  avs::audio::Analysis analysis{};
  analysis.spectrum.fill(0.0f);
  analysis.waveform.fill(0.0f);
  analysis.spectrum[10] = 50.0f;
  auto context = makeContext(pixels, analysis);

  ASSERT_TRUE(effect.render(context));

  bool foundGreen = false;
  for (std::size_t i = 0; i + 3 < pixels.size(); i += 4) {
    const std::uint8_t r = pixels[i];
    const std::uint8_t g = pixels[i + 1u];
    const std::uint8_t b = pixels[i + 2u];
    if (g > r && g > b && g > 0) {
      foundGreen = true;
      break;
    }
  }
  EXPECT_TRUE(foundGreen);
}

TEST(SimpleSpectrumEffectTest, DotScopeUsesWaveformSamples) {
  avs::effects::render::SimpleSpectrum effect;
  avs::core::ParamBlock params;
  params.setInt("effect", (1 << 6) | 2 | (2 << 2) | (2 << 4));
  effect.setParams(params);

  std::vector<std::uint8_t> pixels(
      static_cast<std::size_t>(kWidth) * static_cast<std::size_t>(kHeight) * 4u, 0u);
  avs::audio::Analysis analysis{};
  analysis.spectrum.fill(0.0f);
  analysis.waveform.fill(0.0f);
  for (std::size_t i = 80; i < 120; ++i) {
    analysis.waveform[i] = 1.0f;
  }
  auto context = makeContext(pixels, analysis);

  ASSERT_TRUE(effect.render(context));

  EXPECT_GT(countLitPixels(pixels), 0);
}

