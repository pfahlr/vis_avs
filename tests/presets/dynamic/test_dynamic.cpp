#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <string>
#include <vector>

#include "audio/analyzer.h"
#include "avs/core/EffectRegistry.hpp"
#include "avs/core/ParamBlock.hpp"
#include "avs/core/Pipeline.hpp"
#include "avs/core/RenderContext.hpp"
#include "avs/effects/RegisterEffects.hpp"
#include "md5_helper.hpp"

namespace {

constexpr int kWidth = 96;
constexpr int kHeight = 72;
constexpr int kFrames = 12;

std::filesystem::path goldenDir() {
  return std::filesystem::path(SOURCE_DIR) / "tests/presets/dynamic/golden";
}

bool shouldUpdate() {
  return std::getenv("VIS_AVS_UPDATE_GOLDEN") != nullptr;
}

void writeGolden(const std::filesystem::path& path, const std::string& md5) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::trunc);
  ASSERT_TRUE(out.good());
  out << md5 << '\n';
}

std::string readGolden(const std::filesystem::path& path) {
  std::ifstream in(path);
  if (!in.good()) {
    return {};
  }
  std::string value;
  std::getline(in, value);
  return value;
}

struct RenderResult {
  std::vector<std::uint8_t> pixels;
  std::string md5;
};

RenderResult renderDynamic(const std::string& effectKey,
                           const avs::core::ParamBlock& params,
                           const std::function<void(std::vector<std::uint8_t>&)>& initPattern) {
  avs::core::EffectRegistry registry;
  avs::effects::registerCoreEffects(registry);

  avs::core::Pipeline pipeline(registry);
  pipeline.add(effectKey, params);

  RenderResult result;
  result.pixels.resize(static_cast<std::size_t>(kWidth) * static_cast<std::size_t>(kHeight) * 4u, 0);

  avs::core::RenderContext context;
  context.width = kWidth;
  context.height = kHeight;
  context.framebuffer = {result.pixels.data(), result.pixels.size()};
  context.deltaSeconds = 1.0 / 60.0;

  avs::audio::Analysis analysis{};
  analysis.bass = 0.6f;
  analysis.mid = 0.35f;
  analysis.treb = 0.2f;

  for (int frame = 0; frame < kFrames; ++frame) {
    context.frameIndex = static_cast<std::uint64_t>(frame);
    analysis.beat = (frame % 4) == 0;
    context.audioAnalysis = &analysis;
    if (frame == 0 && initPattern) {
      initPattern(result.pixels);
    }
    if (!pipeline.render(context)) {
      ADD_FAILURE() << "pipeline render failed for frame " << frame;
      break;
    }
  }

  result.md5 = fb_ops_test::computeMd5Hex(result.pixels.data(), result.pixels.size());
  return result;
}

void expectGolden(const std::string& name, const std::string& md5) {
  const auto path = goldenDir() / (name + ".md5");
  if (shouldUpdate()) {
    writeGolden(path, md5);
    GTEST_SKIP() << "Golden updated for " << name;
  }
  const auto golden = readGolden(path);
  ASSERT_FALSE(golden.empty()) << "Missing golden for " << name << " (" << path << ")";
  EXPECT_EQ(md5, golden);
}

void fillRadialDots(std::vector<std::uint8_t>& pixels) {
  std::fill(pixels.begin(), pixels.end(), 0);
  const float cx = (kWidth - 1) * 0.5f;
  const float cy = (kHeight - 1) * 0.5f;
  for (int y = 0; y < kHeight; ++y) {
    for (int x = 0; x < kWidth; ++x) {
      const float dx = static_cast<float>(x) - cx;
      const float dy = static_cast<float>(y) - cy;
      const float dist = std::sqrt(dx * dx + dy * dy);
      if (std::fmod(dist, 7.5f) < 1.0f) {
        const std::size_t idx = (static_cast<std::size_t>(y) * static_cast<std::size_t>(kWidth) +
                                 static_cast<std::size_t>(x)) * 4u;
        pixels[idx + 0] = 220;
        pixels[idx + 1] = 240;
        pixels[idx + 2] = 255;
        pixels[idx + 3] = 255;
      }
    }
  }
}

void fillDiagonalBars(std::vector<std::uint8_t>& pixels) {
  std::fill(pixels.begin(), pixels.end(), 0);
  for (int y = 0; y < kHeight; ++y) {
    for (int x = 0; x < kWidth; ++x) {
      if (((x + y) / 5) % 2 == 0) {
        const std::size_t idx = (static_cast<std::size_t>(y) * static_cast<std::size_t>(kWidth) +
                                 static_cast<std::size_t>(x)) * 4u;
        pixels[idx + 0] = 255;
        pixels[idx + 1] = 80;
        pixels[idx + 2] = 120;
        pixels[idx + 3] = 255;
      }
    }
  }
}

void fillChecker(std::vector<std::uint8_t>& pixels) {
  std::fill(pixels.begin(), pixels.end(), 0);
  for (int y = 0; y < kHeight; ++y) {
    for (int x = 0; x < kWidth; ++x) {
      if (((x / 6) + (y / 6)) % 2 == 0) {
        const std::size_t idx = (static_cast<std::size_t>(y) * static_cast<std::size_t>(kWidth) +
                                 static_cast<std::size_t>(x)) * 4u;
        pixels[idx + 0] = 40;
        pixels[idx + 1] = 200;
        pixels[idx + 2] = 80;
        pixels[idx + 3] = 255;
      }
    }
  }
}

}  // namespace

TEST(DynamicEffectsGoldenTest, DynamicMovementRotatesPattern) {
  avs::core::ParamBlock params;
  params.setString("frame", "q1 = cos(frame*0.12); q2 = sin(frame*0.12);");
  params.setString("pixel", "temp = x*q1 - y*q2; y = x*q2 + y*q1; x = temp;");
  const auto result = renderDynamic("dyn_movement", params, fillRadialDots);
  expectGolden("dyn_movement", result.md5);
}

TEST(DynamicEffectsGoldenTest, DynamicDistanceBreathesPattern) {
  avs::core::ParamBlock params;
  params.setString("frame", "q1 = frame*0.08;");
  params.setString("pixel", "d = d * (0.7 + 0.3*cos(q1 + angle));");
  const auto result = renderDynamic("dyn_distance", params, fillDiagonalBars);
  expectGolden("dyn_distance", result.md5);
}

TEST(DynamicEffectsGoldenTest, DynamicShiftOscillates) {
  avs::core::ParamBlock params;
  params.setString("pixel",
                   "dx = 0.12*sin(frame*0.1 + orig_y*3);"
                   "dy = 0.09*cos(frame*0.07 - orig_x*2);");
  const auto result = renderDynamic("dyn_shift", params, fillChecker);
  expectGolden("dyn_shift", result.md5);
}

TEST(DynamicEffectsGoldenTest, MovementAffineMatrix) {
  avs::core::ParamBlock params;
  params.setFloat("scale", 1.15f);
  params.setFloat("rotate", 18.0f);
  params.setFloat("offset_x", -0.25f);
  params.setFloat("offset_y", 0.18f);
  params.setBool("wrap", false);
  const auto result = renderDynamic("movement", params, fillChecker);
  expectGolden("movement", result.md5);
}

TEST(DynamicEffectsGoldenTest, ZoomRotateAnchored) {
  avs::core::ParamBlock params;
  params.setFloat("zoom", 1.35f);
  params.setFloat("rotate", 42.0f);
  params.setFloat("anchor_x", 0.3f);
  params.setFloat("anchor_y", 0.65f);
  params.setBool("wrap", true);
  const auto result = renderDynamic("zoom_rotate", params, fillDiagonalBars);
  expectGolden("zoom_rotate", result.md5);
}

