#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <cmath>
#include <vector>

#include <avs/core/ParamBlock.hpp>
#include <avs/core/RenderContext.hpp>
#include "effects/trans/effect_blitter_feedback.h"

namespace {

std::size_t pixelIndex(int x, int y, int width) {
  return (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
          static_cast<std::size_t>(x)) * 4u;
}

void setPixel(std::vector<std::uint8_t>& pixels,
              int width,
              int x,
              int y,
              std::uint8_t r,
              std::uint8_t g,
              std::uint8_t b,
              std::uint8_t a = 255) {
  const std::size_t index = pixelIndex(x, y, width);
  pixels[index + 0] = r;
  pixels[index + 1] = g;
  pixels[index + 2] = b;
  pixels[index + 3] = a;
}

std::array<std::uint8_t, 4> getPixel(const std::vector<std::uint8_t>& pixels, int width, int x, int y) {
  const std::size_t index = pixelIndex(x, y, width);
  return {pixels[index + 0], pixels[index + 1], pixels[index + 2], pixels[index + 3]};
}

void initialiseContext(avs::core::RenderContext& context,
                       std::vector<std::uint8_t>& pixels,
                       int width,
                       int height) {
  context = {};
  context.width = width;
  context.height = height;
  context.framebuffer = {pixels.data(), pixels.size()};
}

struct DiscreteTransform {
  bool mirrorX = false;
  bool mirrorY = false;
  int rotateQuadrants = 0;
};

std::pair<int, int> transformCoordinates(int x, int y, int width, int height, const DiscreteTransform& transform) {
  const int centerX = (width - 1) / 2;
  const int centerY = (height - 1) / 2;

  int nx = x - centerX;
  int ny = centerY - y;  // Positive upwards.

  if (transform.mirrorX) {
    nx = -nx;
  }
  if (transform.mirrorY) {
    ny = -ny;
  }

  int rx = nx;
  int ry = ny;
  for (int i = 0; i < transform.rotateQuadrants; ++i) {
    const int tmp = rx;
    rx = ry;
    ry = -tmp;
  }

  const int sourceX = rx + centerX;
  const int sourceY = centerY - ry;
  return {sourceX, sourceY};
}

void populateGrid(std::vector<std::uint8_t>& pixels, int width, int height) {
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const std::uint8_t base = static_cast<std::uint8_t>(10 + y * width + x);
      setPixel(pixels, width, x, y, base, static_cast<std::uint8_t>(base + 40),
               static_cast<std::uint8_t>(base + 80));
    }
  }
}

}  // namespace

TEST(BlitterFeedbackTest, MirrorsHorizontally) {
  constexpr int kWidth = 3;
  constexpr int kHeight = 3;
  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(kWidth) * static_cast<std::size_t>(kHeight) * 4u, 0);
  populateGrid(pixels, kWidth, kHeight);

  avs::effects::trans::BlitterFeedback effect;
  avs::core::ParamBlock params;
  params.setBool("mirror_x", true);
  effect.setParams(params);

  avs::core::RenderContext context{};
  initialiseContext(context, pixels, kWidth, kHeight);

  const auto originalPixels = pixels;

  ASSERT_TRUE(effect.render(context));

  for (int y = 0; y < kHeight; ++y) {
    for (int x = 0; x < kWidth; ++x) {
      const auto source = transformCoordinates(x, y, kWidth, kHeight, DiscreteTransform{true, false, 0});
      ASSERT_GE(source.first, 0);
      ASSERT_LT(source.first, kWidth);
      ASSERT_GE(source.second, 0);
      ASSERT_LT(source.second, kHeight);
      const auto output = getPixel(pixels, kWidth, x, y);
      const auto expected = getPixel(originalPixels, kWidth, source.first, source.second);
      for (int channel = 0; channel < 3; ++channel) {
        EXPECT_NEAR(static_cast<double>(output[static_cast<std::size_t>(channel)]),
                    static_cast<double>(expected[static_cast<std::size_t>(channel)]),
                    1.0);
      }
      EXPECT_EQ(output[3], expected[3]);
    }
  }
}

TEST(BlitterFeedbackTest, RotatesClockwise) {
  constexpr int kWidth = 3;
  constexpr int kHeight = 3;
  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(kWidth) * static_cast<std::size_t>(kHeight) * 4u, 0);
  populateGrid(pixels, kWidth, kHeight);

  avs::effects::trans::BlitterFeedback effect;
  avs::core::ParamBlock params;
  params.setInt("rotate_quadrants", 1);
  effect.setParams(params);

  avs::core::RenderContext context{};
  initialiseContext(context, pixels, kWidth, kHeight);

  const auto originalPixels = pixels;

  ASSERT_TRUE(effect.render(context));

  const DiscreteTransform transform{false, false, 1};
  for (int y = 0; y < kHeight; ++y) {
    for (int x = 0; x < kWidth; ++x) {
      const auto source = transformCoordinates(x, y, kWidth, kHeight, transform);
      ASSERT_GE(source.first, 0);
      ASSERT_LT(source.first, kWidth);
      ASSERT_GE(source.second, 0);
      ASSERT_LT(source.second, kHeight);
      const auto output = getPixel(pixels, kWidth, x, y);
      const auto expected = getPixel(originalPixels, kWidth, source.first, source.second);
      for (int channel = 0; channel < 3; ++channel) {
        EXPECT_NEAR(static_cast<double>(output[static_cast<std::size_t>(channel)]),
                    static_cast<double>(expected[static_cast<std::size_t>(channel)]),
                    1.0);
      }
      EXPECT_EQ(output[3], expected[3]);
    }
  }
}

TEST(BlitterFeedbackTest, FeedbackGainClampsValues) {
  constexpr int kWidth = 3;
  constexpr int kHeight = 3;
  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(kWidth) * static_cast<std::size_t>(kHeight) * 4u, 0);
  populateGrid(pixels, kWidth, kHeight);

  avs::effects::trans::BlitterFeedback effect;
  avs::core::ParamBlock params;
  params.setFloat("feedback", 0.5f);
  effect.setParams(params);

  avs::core::RenderContext context{};
  initialiseContext(context, pixels, kWidth, kHeight);

  const auto originalPixels = pixels;

  ASSERT_TRUE(effect.render(context));

  for (int y = 0; y < kHeight; ++y) {
    for (int x = 0; x < kWidth; ++x) {
      const auto sourcePixel = transformCoordinates(x, y, kWidth, kHeight, DiscreteTransform{});
      const auto original = getPixel(originalPixels, kWidth, sourcePixel.first, sourcePixel.second);
      const auto current = getPixel(pixels, kWidth, x, y);
      for (int channel = 0; channel < 3; ++channel) {
        const float expected = std::round(static_cast<float>(original[static_cast<std::size_t>(channel)]) * 0.5f);
        EXPECT_NEAR(static_cast<float>(current[static_cast<std::size_t>(channel)]), expected, 1.0f);
      }
      EXPECT_EQ(current[3], original[3]);
    }
  }
}

