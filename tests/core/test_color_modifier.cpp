#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>

#include "avs/core/ParamBlock.hpp"
#include "avs/core/RenderContext.hpp"
#include "effects/trans/effect_color_modifier.h"
#include "gtest/gtest.h"

namespace {

float normalizedValue(std::uint8_t value) { return static_cast<float>(value) / 255.0f; }

std::uint8_t clampToByte(float normalized) {
  const float clamped = std::clamp(normalized, 0.0f, 1.0f);
  const int scaled = static_cast<int>(std::lround(clamped * 255.0f));
  return static_cast<std::uint8_t>(std::clamp(scaled, 0, 255));
}

std::uint8_t sineTransform(std::uint8_t value) {
  const float centered = normalizedValue(value) - 0.5f;
  const float sine = std::sin(centered * std::numbers::pi_v<float>);
  return clampToByte(0.5f * (sine + 1.0f));
}

std::uint8_t cosineTransform(std::uint8_t value) {
  const float centered = normalizedValue(value) - 0.5f;
  const float cosine = std::cos(centered * std::numbers::pi_v<float>);
  return clampToByte(0.5f * (cosine + 1.0f));
}

std::uint8_t mixTransform(std::uint8_t value) {
  const float sine = static_cast<float>(sineTransform(value)) / 255.0f;
  const float cosine = static_cast<float>(cosineTransform(value)) / 255.0f;
  return clampToByte(0.5f * (sine + cosine));
}

avs::core::RenderContext makeContext(std::array<std::uint8_t, 4>& pixel) {
  avs::core::RenderContext context{};
  context.width = 1;
  context.height = 1;
  context.framebuffer = {pixel.data(), pixel.size()};
  return context;
}

}  // namespace

TEST(ColorModifierEffect, AppliesSineModeByDefault) {
  avs::effects::trans::ColorModifier effect;
  std::array<std::uint8_t, 4> pixel{0u, 128u, 255u, 90u};
  auto context = makeContext(pixel);

  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(pixel[0], sineTransform(0u));
  EXPECT_EQ(pixel[1], sineTransform(128u));
  EXPECT_EQ(pixel[2], sineTransform(255u));
  EXPECT_EQ(pixel[3], 90u);
}

TEST(ColorModifierEffect, ParsesStringModes) {
  avs::effects::trans::ColorModifier effect;
  avs::core::ParamBlock params;
  params.setString("mode", "cosine");
  effect.setParams(params);

  std::array<std::uint8_t, 4> pixel{60u, 150u, 200u, 255u};
  auto context = makeContext(pixel);

  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(pixel[0], cosineTransform(60u));
  EXPECT_EQ(pixel[1], cosineTransform(150u));
  EXPECT_EQ(pixel[2], cosineTransform(200u));
}

TEST(ColorModifierEffect, SupportsSineCosineMode) {
  avs::effects::trans::ColorModifier effect;
  avs::core::ParamBlock params;
  params.setInt("mode", static_cast<int>(avs::effects::trans::ColorModifier::Mode::SineCosine));
  effect.setParams(params);

  std::array<std::uint8_t, 4> pixel{32u, 96u, 160u, 12u};
  auto context = makeContext(pixel);

  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(pixel[0], sineTransform(32u));
  EXPECT_EQ(pixel[1], cosineTransform(96u));
  EXPECT_EQ(pixel[2], mixTransform(160u));
  EXPECT_EQ(pixel[3], 12u);
}

TEST(ColorModifierEffect, HonorsDisabledFlag) {
  avs::effects::trans::ColorModifier effect;
  avs::core::ParamBlock params;
  params.setBool("enabled", false);
  effect.setParams(params);

  std::array<std::uint8_t, 4> pixel{10u, 20u, 30u, 40u};
  auto context = makeContext(pixel);

  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(pixel[0], 10u);
  EXPECT_EQ(pixel[1], 20u);
  EXPECT_EQ(pixel[2], 30u);
  EXPECT_EQ(pixel[3], 40u);
}
