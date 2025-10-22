#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>

#include "audio/analyzer.h"
#include "avs/core/ParamBlock.hpp"
#include "avs/core/RenderContext.hpp"
#include "avs/runtime/GlobalState.hpp"
#include "effects/render/effect_timescope.h"

namespace {

template <std::size_t N>
avs::core::RenderContext makeContext(int width, int height, std::array<std::uint8_t, N>& buffer,
                                     avs::runtime::GlobalState& globals,
                                     avs::audio::Analysis* analysis) {
  avs::core::RenderContext context{};
  context.width = width;
  context.height = height;
  context.framebuffer = {buffer.data(), buffer.size()};
  context.audioAnalysis = analysis;
  if (analysis) {
    context.audioSpectrum = {analysis->spectrum.data(), analysis->spectrum.size()};
  } else {
    context.audioSpectrum = {nullptr, 0};
  }
  context.globals = &globals;
  return context;
}

}  // namespace

TEST(RenderTimescopeTest, WritesWaveformColumn) {
  avs::effects::render::Timescope effect;
  avs::core::ParamBlock params;
  params.setInt("nbands", 4);
  effect.setParams(params);

  avs::audio::Analysis analysis{};
  analysis.waveform.fill(0.0f);
  analysis.waveform[0] = 1.0f;
  analysis.waveform[144] = 0.5f;
  analysis.waveform[288] = -0.75f;
  analysis.waveform[432] = 0.0f;

  avs::runtime::GlobalState globals{};
  std::array<std::uint8_t, 4 * 4 * 4> framebuffer{};
  auto context = makeContext(4, 4, framebuffer, globals, &analysis);

  ASSERT_TRUE(effect.render(context));

  const std::size_t rowStride = static_cast<std::size_t>(context.width) * 4u;
  auto pixel = [&](int row) {
    const std::size_t index = static_cast<std::size_t>(row) * rowStride;
    return std::array<std::uint8_t, 4>{framebuffer[index], framebuffer[index + 1],
                                       framebuffer[index + 2], framebuffer[index + 3]};
  };

  EXPECT_EQ(pixel(0)[0], 255u);
  EXPECT_EQ(pixel(0)[1], 255u);
  EXPECT_EQ(pixel(0)[2], 255u);
  EXPECT_EQ(pixel(1)[0], 128u);
  EXPECT_EQ(pixel(1)[1], 128u);
  EXPECT_EQ(pixel(1)[2], 128u);
  EXPECT_EQ(pixel(2)[0], 191u);
  EXPECT_EQ(pixel(2)[1], 191u);
  EXPECT_EQ(pixel(2)[2], 191u);
  EXPECT_EQ(pixel(3)[0], 0u);
  EXPECT_EQ(pixel(3)[1], 0u);
  EXPECT_EQ(pixel(3)[2], 0u);
}

TEST(RenderTimescopeTest, AdditiveBlendSaturates) {
  avs::effects::render::Timescope effect;
  avs::core::ParamBlock params;
  params.setInt("blend", 1);
  params.setInt("nbands", 2);
  effect.setParams(params);

  avs::audio::Analysis analysis{};
  analysis.waveform.fill(0.0f);
  analysis.waveform[0] = 0.6f;
  analysis.waveform[288] = 0.9f;

  avs::runtime::GlobalState globals{};
  std::array<std::uint8_t, 4 * 2 * 4> framebuffer{};
  for (std::size_t i = 0; i < framebuffer.size(); ++i) {
    framebuffer[i] = 200u;
  }
  auto context = makeContext(4, 2, framebuffer, globals, &analysis);

  ASSERT_TRUE(effect.render(context));

  const std::size_t stride = static_cast<std::size_t>(context.width) * 4u;
  EXPECT_EQ(framebuffer[0], 255u);
  EXPECT_EQ(framebuffer[1], 255u);
  EXPECT_EQ(framebuffer[2], 255u);
  EXPECT_EQ(framebuffer[stride], 255u);
  EXPECT_EQ(framebuffer[stride + 1], 255u);
  EXPECT_EQ(framebuffer[stride + 2], 255u);
}

TEST(RenderTimescopeTest, HandlesMissingAudioGracefully) {
  avs::effects::render::Timescope effect;
  avs::runtime::GlobalState globals{};
  std::array<std::uint8_t, 4 * 3 * 4> framebuffer{};
  framebuffer.fill(42u);
  auto context = makeContext(4, 3, framebuffer, globals, nullptr);

  ASSERT_TRUE(effect.render(context));

  const std::size_t stride = static_cast<std::size_t>(context.width) * 4u;
  for (int row = 0; row < context.height; ++row) {
    const std::size_t base = static_cast<std::size_t>(row) * stride;
    EXPECT_EQ(framebuffer[base + 0], 0u);
    EXPECT_EQ(framebuffer[base + 1], 0u);
    EXPECT_EQ(framebuffer[base + 2], 0u);
    EXPECT_EQ(framebuffer[base + 3], 255u);
    for (int column = 1; column < context.width; ++column) {
      const std::size_t offset = base + static_cast<std::size_t>(column) * 4u;
      EXPECT_EQ(framebuffer[offset + 0], 42u);
      EXPECT_EQ(framebuffer[offset + 1], 42u);
      EXPECT_EQ(framebuffer[offset + 2], 42u);
      EXPECT_EQ(framebuffer[offset + 3], 42u);
    }
  }
}
