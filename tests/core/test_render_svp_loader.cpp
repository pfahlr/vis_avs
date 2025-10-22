#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

#include "audio/analyzer.h"
#include "avs/core/ParamBlock.hpp"
#include "avs/core/RenderContext.hpp"
#include "effects/render/effect_svp_loader.h"

namespace {

constexpr int kWidth = 32;
constexpr int kHeight = 24;

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

}  // namespace

TEST(SvpLoaderEffectTest, DoesNothingWhenNoLibraryIsConfigured) {
  avs::effects::render::SvpLoader effect;
  std::vector<std::uint8_t> pixels(
      static_cast<std::size_t>(kWidth) * static_cast<std::size_t>(kHeight) * 4u, 42u);
  avs::audio::Analysis analysis{};
  auto context = makeContext(pixels, analysis);

  EXPECT_TRUE(effect.render(context));

  for (std::uint8_t value : pixels) {
    EXPECT_EQ(value, 42u);
  }
}

TEST(SvpLoaderEffectTest, HandlesMissingLibraryGracefully) {
  avs::effects::render::SvpLoader effect;
  avs::core::ParamBlock params;
  params.setString("library", "definitely_missing_plugin.svp");
  effect.setParams(params);

  std::vector<std::uint8_t> pixels(
      static_cast<std::size_t>(kWidth) * static_cast<std::size_t>(kHeight) * 4u, 17u);
  avs::audio::Analysis analysis{};
  analysis.spectrum.fill(0.0f);
  analysis.waveform.fill(0.0f);
  auto context = makeContext(pixels, analysis);

  EXPECT_TRUE(effect.render(context));

  for (std::uint8_t value : pixels) {
    EXPECT_EQ(value, 17u);
  }
}

