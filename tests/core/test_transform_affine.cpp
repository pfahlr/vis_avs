#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <avs/core/EffectRegistry.hpp>
#include <avs/core/ParamBlock.hpp>
#include <avs/core/Pipeline.hpp>
#include <avs/core/RenderContext.hpp>
#include <avs/effects/RegisterEffects.hpp>
#include <avs/effects/TransformAffine.hpp>
#include <avs/effects/micro_preset_parser.hpp>
#include <avs/effects/legacy/transform_affine.h>

namespace {

using avs::effects::Affine2D;
using avs::effects::TransformAffine;

avs::core::RenderContext makeContext(std::vector<std::uint8_t>& pixels, int width, int height) {
  avs::core::RenderContext ctx;
  ctx.frameIndex = 0;
  ctx.deltaSeconds = 1.0 / 60.0;
  ctx.width = width;
  ctx.height = height;
  ctx.framebuffer = {pixels.data(), pixels.size()};
  ctx.audioSpectrum = {nullptr, 0};
  ctx.audioBeat = false;
  return ctx;
}

std::array<std::uint8_t, 4> readPixel(const avs::core::RenderContext& ctx, int x, int y) {
  const std::size_t offset =
      (static_cast<std::size_t>(y) * static_cast<std::size_t>(ctx.width) + static_cast<std::size_t>(x)) * 4u;
  EXPECT_LT(offset + 3, ctx.framebuffer.size);
  return {ctx.framebuffer.data[offset + 0], ctx.framebuffer.data[offset + 1], ctx.framebuffer.data[offset + 2],
          ctx.framebuffer.data[offset + 3]};
}

bool pixelEquals(const std::array<std::uint8_t, 4>& pixel, std::uint8_t r, std::uint8_t g, std::uint8_t b) {
  return pixel[0] == r && pixel[1] == g && pixel[2] == b;
}

void zeroBuffer(std::vector<std::uint8_t>& pixels) {
  std::fill(pixels.begin(), pixels.end(), 0);
}

std::string loadFile(const std::filesystem::path& path) {
  std::ifstream file(path);
  if (!file) {
    throw std::runtime_error("failed to read file: " + path.string());
  }
  std::ostringstream stream;
  stream << file.rdbuf();
  return stream.str();
}

}  // namespace

TEST(Affine2D, ComposesTranslationRotation) {
  constexpr float kPi = 3.14159265358979323846f;
  Affine2D translate = Affine2D::translation(3.0f, -2.0f);
  Affine2D rotate = Affine2D::rotation(kPi / 2.0f);
  Affine2D combined = translate * rotate;
  auto result = combined.apply({1.0f, 0.0f});
  EXPECT_NEAR(result[0], 3.0f, 1e-3f);
  EXPECT_NEAR(result[1], -1.0f, 1e-3f);
}

class TransformAffinePipelineTest : public ::testing::Test {
 protected:
  TransformAffinePipelineTest() { avs::effects::registerCoreEffects(registry_); }

  avs::core::EffectRegistry registry_;
};

TEST_F(TransformAffinePipelineTest, DrawsCrosshairAtCenter) {
  avs::core::Pipeline pipeline(registry_);
  pipeline.add("clear", avs::core::ParamBlock{});
  avs::core::ParamBlock params;
  params.setBool("test", true);
  params.setString("anchor", "center");
  pipeline.add("transform_affine", params);

  constexpr int kSize = 32;
  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(kSize * kSize * 4), 0);
  auto ctx = makeContext(pixels, kSize, kSize);
  ASSERT_TRUE(pipeline.render(ctx));

  const int center = kSize / 2;
  auto pixel = readPixel(ctx, center, center);
  EXPECT_TRUE(pixelEquals(pixel, 255, 255, 255));
}

TEST_F(TransformAffinePipelineTest, RotatesTriangleAroundAnchor) {
  avs::core::Pipeline pipeline(registry_);
  pipeline.add("clear", avs::core::ParamBlock{});
  avs::core::ParamBlock params;
  params.setBool("test", true);
  params.setString("anchor", "center");
  params.setFloat("rotate", 90.0f);
  params.setInt("color", 0x0000FF00);
  pipeline.add("transform_affine", params);

  constexpr int kSize = 48;
  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(kSize * kSize * 4), 0);
  auto ctx = makeContext(pixels, kSize, kSize);

  ASSERT_TRUE(pipeline.render(ctx));
  const int center = kSize / 2;
  auto upPixel = readPixel(ctx, center, center - 8);
  EXPECT_TRUE(pixelEquals(upPixel, 0, 255, 0));

  zeroBuffer(pixels);
  ctx.frameIndex = 1;
  ctx.framebuffer = {pixels.data(), pixels.size()};
  ASSERT_TRUE(pipeline.render(ctx));
  auto rightPixel = readPixel(ctx, center + 8, center);
  EXPECT_TRUE(pixelEquals(rightPixel, 0, 255, 0));
}

TEST_F(TransformAffinePipelineTest, StickyGatingLogTracksStates) {
  avs::core::Pipeline pipeline(registry_);
  pipeline.add("clear", avs::core::ParamBlock{});
  avs::core::ParamBlock params;
  params.setBool("test", true);
  params.setString("anchor", "center");
  params.setBool("onbeat", true);
  params.setBool("stick", true);
  params.setBool("onlysticky", true);
  params.setInt("log_rows", 1);
  pipeline.add("transform_affine", params);

  constexpr int kSize = 24;
  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(kSize * kSize * 4), 0);
  auto ctx = makeContext(pixels, kSize, kSize);

  ctx.audioBeat = true;
  ASSERT_TRUE(pipeline.render(ctx));
  auto bottomBeat = readPixel(ctx, kSize - 1, kSize - 1);
  EXPECT_TRUE(pixelEquals(bottomBeat, 220, 220, 40));

  zeroBuffer(pixels);
  ctx.frameIndex = 1;
  ctx.audioBeat = false;
  ctx.framebuffer = {pixels.data(), pixels.size()};
  ASSERT_TRUE(pipeline.render(ctx));
  auto bottomSticky = readPixel(ctx, kSize - 1, kSize - 1);
  EXPECT_TRUE(pixelEquals(bottomSticky, 220, 220, 40));

  zeroBuffer(pixels);
  ctx.frameIndex = 2;
  ctx.audioBeat = true;
  ctx.framebuffer = {pixels.data(), pixels.size()};
  ASSERT_TRUE(pipeline.render(ctx));
  auto bottomOffBeat = readPixel(ctx, kSize - 1, kSize - 1);
  EXPECT_TRUE(pixelEquals(bottomOffBeat, 200, 40, 40));

  zeroBuffer(pixels);
  ctx.frameIndex = 3;
  ctx.audioBeat = false;
  ctx.framebuffer = {pixels.data(), pixels.size()};
  ASSERT_TRUE(pipeline.render(ctx));
  auto bottomInactive = readPixel(ctx, kSize - 1, kSize - 1);
  EXPECT_TRUE(pixelEquals(bottomInactive, 24, 24, 24));
}

TEST_F(TransformAffinePipelineTest, HoldFramesEmitHoldColor) {
  avs::core::Pipeline pipeline(registry_);
  pipeline.add("clear", avs::core::ParamBlock{});
  avs::core::ParamBlock params;
  params.setBool("test", true);
  params.setString("anchor", "center");
  params.setBool("onbeat", true);
  params.setInt("hold", 2);
  params.setInt("log_rows", 1);
  pipeline.add("transform_affine", params);

  constexpr int kSize = 16;
  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(kSize * kSize * 4), 0);
  auto ctx = makeContext(pixels, kSize, kSize);

  ctx.audioBeat = true;
  ASSERT_TRUE(pipeline.render(ctx));
  auto beatColor = readPixel(ctx, kSize - 1, kSize - 1);
  EXPECT_TRUE(pixelEquals(beatColor, 200, 40, 40));

  zeroBuffer(pixels);
  ctx.frameIndex = 1;
  ctx.audioBeat = false;
  ctx.framebuffer = {pixels.data(), pixels.size()};
  ASSERT_TRUE(pipeline.render(ctx));
  auto holdColor = readPixel(ctx, kSize - 1, kSize - 1);
  EXPECT_TRUE(pixelEquals(holdColor, 40, 160, 40));

  zeroBuffer(pixels);
  ctx.frameIndex = 2;
  ctx.audioBeat = false;
  ctx.framebuffer = {pixels.data(), pixels.size()};
  ASSERT_TRUE(pipeline.render(ctx));
  auto offColor = readPixel(ctx, kSize - 1, kSize - 1);
  EXPECT_TRUE(pixelEquals(offColor, 24, 24, 24));
}

TEST_F(TransformAffinePipelineTest, GatingLogStacksAcrossRows) {
  avs::core::Pipeline pipeline(registry_);
  pipeline.add("clear", avs::core::ParamBlock{});
  avs::core::ParamBlock params;
  params.setBool("onbeat", true);
  params.setInt("hold", 2);
  params.setInt("log_rows", 2);
  pipeline.add("transform_affine", params);

  constexpr int kSize = 4;
  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(kSize * kSize * 4), 0);
  auto ctx = makeContext(pixels, kSize, kSize);

  const std::array<bool, 6> beats{true, false, false, true, false, false};
  for (std::size_t i = 0; i < beats.size(); ++i) {
    ctx.frameIndex = i;
    ctx.audioBeat = beats[i];
    ASSERT_TRUE(pipeline.render(ctx));
  }

  auto bottomLeft = readPixel(ctx, 0, kSize - 1);
  EXPECT_TRUE(pixelEquals(bottomLeft, 24, 24, 24));

  auto bottomBeat = readPixel(ctx, 1, kSize - 1);
  EXPECT_TRUE(pixelEquals(bottomBeat, 200, 40, 40));

  auto bottomHold = readPixel(ctx, 2, kSize - 1);
  EXPECT_TRUE(pixelEquals(bottomHold, 40, 160, 40));

  auto upperBeat = readPixel(ctx, kSize - 2, kSize - 2);
  EXPECT_TRUE(pixelEquals(upperBeat, 200, 40, 40));

  auto upperHold = readPixel(ctx, kSize - 1, kSize - 2);
  EXPECT_TRUE(pixelEquals(upperHold, 40, 160, 40));
}

TEST_F(TransformAffinePipelineTest, MicroPresetRotateCenter) {
  const auto presetPath = std::filesystem::path(SOURCE_DIR) / "tests/presets/transforms/rotate_center.txt";
  const auto text = loadFile(presetPath);
  const auto preset = avs::effects::parseMicroPreset(text);
  EXPECT_TRUE(preset.warnings.empty());
  avs::core::Pipeline pipeline(registry_);
  for (const auto& command : preset.commands) {
    pipeline.add(command.effectKey, command.params);
  }

  constexpr int kSize = 48;
  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(kSize * kSize * 4), 0);
  auto ctx = makeContext(pixels, kSize, kSize);
  ASSERT_TRUE(pipeline.render(ctx));

  const int center = kSize / 2;
  auto upPixel = readPixel(ctx, center, center - 8);
  EXPECT_TRUE(pixelEquals(upPixel, 0, 255, 0));
}

TEST_F(TransformAffinePipelineTest, MicroPresetStickyOnly) {
  const auto presetPath = std::filesystem::path(SOURCE_DIR) / "tests/presets/gating/sticky_only.txt";
  const auto text = loadFile(presetPath);
  const auto preset = avs::effects::parseMicroPreset(text);
  EXPECT_TRUE(preset.warnings.empty());
  avs::core::Pipeline pipeline(registry_);
  for (const auto& command : preset.commands) {
    pipeline.add(command.effectKey, command.params);
  }

  constexpr int kSize = 24;
  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(kSize * kSize * 4), 0);
  auto ctx = makeContext(pixels, kSize, kSize);

  ctx.audioBeat = true;
  ASSERT_TRUE(pipeline.render(ctx));
  auto stickyColor = readPixel(ctx, kSize - 1, kSize - 1);
  EXPECT_TRUE(pixelEquals(stickyColor, 220, 220, 40));

  zeroBuffer(pixels);
  ctx.frameIndex = 1;
  ctx.audioBeat = true;
  ctx.framebuffer = {pixels.data(), pixels.size()};
  ASSERT_TRUE(pipeline.render(ctx));
  auto beatColor = readPixel(ctx, kSize - 1, kSize - 1);
  EXPECT_TRUE(pixelEquals(beatColor, 200, 40, 40));
}

TEST_F(TransformAffinePipelineTest, RandomPositionDeterministicWithSeed) {
  ASSERT_EQ(setenv("VIS_AVS_SEED", "4242", 1), 0);
  auto makePipeline = [this]() {
    avs::core::Pipeline pipe(registry_);
    pipe.add("clear", avs::core::ParamBlock{});
    avs::core::ParamBlock params;
    params.setBool("test", true);
    params.setString("anchor", "center");
    params.setBool("onbeat", true);
    params.setInt("hold", 1);
    params.setBool("randompos", true);
    params.setFloat("random_offset", 0.1f);
    pipe.add("transform_affine", params);
    return pipe;
  };

  constexpr int kSize = 40;
  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(kSize * kSize * 4), 0);
  auto ctx = makeContext(pixels, kSize, kSize);

  auto pipeline = makePipeline();
  ctx.audioBeat = true;
  ASSERT_TRUE(pipeline.render(ctx));

  int firstX = -1;
  int firstY = -1;
  for (int y = 0; y < kSize; ++y) {
    for (int x = 0; x < kSize; ++x) {
      auto pixel = readPixel(ctx, x, y);
      if (pixelEquals(pixel, 255, 255, 255)) {
        firstX = x;
        firstY = y;
      }
    }
  }
  ASSERT_NE(firstX, -1);
  ASSERT_NE(firstY, -1);

  zeroBuffer(pixels);
  auto pipeline2 = makePipeline();
  ctx = makeContext(pixels, kSize, kSize);
  ctx.audioBeat = true;
  ASSERT_TRUE(pipeline2.render(ctx));

  int secondX = -1;
  int secondY = -1;
  for (int y = 0; y < kSize; ++y) {
    for (int x = 0; x < kSize; ++x) {
      auto pixel = readPixel(ctx, x, y);
      if (pixelEquals(pixel, 255, 255, 255)) {
        secondX = x;
        secondY = y;
      }
    }
  }
  EXPECT_EQ(firstX, secondX);
  EXPECT_EQ(firstY, secondY);
}

