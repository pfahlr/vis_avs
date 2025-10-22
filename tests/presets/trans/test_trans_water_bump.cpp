#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "avs/core/DeterministicRng.hpp"
#include "avs/core/EffectRegistry.hpp"
#include "avs/core/ParamBlock.hpp"
#include "avs/core/Pipeline.hpp"
#include "avs/core/RenderContext.hpp"
#include "avs/effects/RegisterEffects.hpp"
#include "avs/offscreen/Md5.hpp"
#include "effects/trans/effect_water_bump.h"

namespace {

constexpr int kWidth = 64;
constexpr int kHeight = 48;
constexpr int kFrames = 6;
constexpr std::array<bool, kFrames> kBeatPattern{true, false, false, true, false, false};

struct FrameHashResult {
  std::vector<std::uint8_t> pixels;
  std::vector<std::string> hashes;
};

std::vector<std::uint8_t> makeBasePattern() {
  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(kWidth) *
                                   static_cast<std::size_t>(kHeight) * 4u);
  for (int y = 0; y < kHeight; ++y) {
    for (int x = 0; x < kWidth; ++x) {
      const std::size_t index =
          (static_cast<std::size_t>(y) * kWidth + static_cast<std::size_t>(x)) * 4u;
      const int r = (x * 19 + y * 23) & 0xFF;
      const int g = (x * 11 + y * 41 + 73) & 0xFF;
      const int b = (x * 37 + y * 13 + 17) & 0xFF;
      pixels[index + 0] = static_cast<std::uint8_t>(r);
      pixels[index + 1] = static_cast<std::uint8_t>(g);
      pixels[index + 2] = static_cast<std::uint8_t>(b);
      pixels[index + 3] = 255u;
    }
  }
  return pixels;
}

FrameHashResult renderWaterBump(const avs::core::ParamBlock& params) {
  avs::core::EffectRegistry registry;
  avs::effects::registerCoreEffects(registry);

  avs::core::Pipeline pipeline(registry);
  pipeline.add("trans / water bump", params);

  FrameHashResult result;
  result.pixels = makeBasePattern();
  std::vector<std::uint8_t> working = result.pixels;

  avs::core::RenderContext context{};
  context.width = kWidth;
  context.height = kHeight;
  context.deltaSeconds = 1.0 / 60.0;
  context.framebuffer = {working.data(), working.size()};

  result.hashes.reserve(kFrames);
  for (int frame = 0; frame < kFrames; ++frame) {
    context.frameIndex = static_cast<std::uint64_t>(frame);
    context.audioBeat = kBeatPattern[frame];
    std::copy(result.pixels.begin(), result.pixels.end(), working.begin());
    pipeline.render(context);
    result.hashes.push_back(avs::offscreen::computeMd5Hex(working.data(), working.size()));
  }

  std::swap(result.pixels, working);
  return result;
}

std::filesystem::path goldenDir() {
  return std::filesystem::path(SOURCE_DIR) / "tests" / "presets" / "trans" / "water_bump";
}

std::vector<std::string> loadGolden(const std::filesystem::path& path) {
  std::vector<std::string> hashes;
  std::ifstream in(path);
  if (!in.is_open()) {
    return hashes;
  }
  std::string line;
  while (std::getline(in, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    if (!line.empty()) {
      hashes.push_back(line);
    }
  }
  return hashes;
}

void expectGolden(const std::string& name, const std::vector<std::string>& hashes) {
  const auto path = goldenDir() / name / "hashes.md5";
  const auto golden = loadGolden(path);
  if (golden.empty()) {
    ADD_FAILURE() << "Missing golden hashes for " << name << " at " << path
                  << "\nCaptured hashes:\n";
    for (const auto& hash : hashes) {
      ADD_FAILURE() << hash;
    }
    return;
  }
  ASSERT_EQ(golden.size(), hashes.size()) << "Golden/hash count mismatch for " << name;
  EXPECT_EQ(golden, hashes) << "Golden mismatch for " << name;
}

class WaterBumpEffectTests : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
#ifdef _WIN32
    _putenv_s("VIS_AVS_SEED", "20240523");
#else
    ::setenv("VIS_AVS_SEED", "20240523", 1);
#endif
  }
};

}  // namespace

TEST_F(WaterBumpEffectTests, DefaultGolden) {
  avs::core::ParamBlock params;
  params.setBool("enabled", true);
  params.setBool("random_drop", false);
  params.setInt("drop_radius", 40);
  params.setInt("density", 6);
  params.setInt("depth", 600);
  const auto result = renderWaterBump(params);
  expectGolden("default", result.hashes);
}

TEST(WaterBumpEffect, BeatTriggersDisplacement) {
  avs::effects::trans::WaterBump effect;
  avs::core::ParamBlock params;
  params.setBool("random_drop", false);
  params.setInt("drop_position_x", 1);
  params.setInt("drop_position_y", 1);
  params.setInt("drop_radius", 50);
  params.setInt("depth", 800);
  effect.setParams(params);

  std::array<std::uint8_t, 4 * 9> pixels{};
  for (int i = 0; i < 9; ++i) {
    pixels[i * 4 + 0] = static_cast<std::uint8_t>(10 + i * 20);
    pixels[i * 4 + 1] = static_cast<std::uint8_t>(5 + i * 15);
    pixels[i * 4 + 2] = static_cast<std::uint8_t>(200 - i * 10);
    pixels[i * 4 + 3] = 255u;
  }

  avs::core::RenderContext context{};
  context.width = 3;
  context.height = 3;
  context.framebuffer = {pixels.data(), pixels.size()};
  context.audioBeat = false;
  context.frameIndex = 0;
  context.rng = avs::core::DeterministicRng(1234);
  context.rng.reseed(context.frameIndex);

  const auto original = pixels;
  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(pixels, original) << "No beat should leave the frame untouched.";

  context.audioBeat = true;
  context.frameIndex = 1;
  context.rng.reseed(context.frameIndex);
  ASSERT_TRUE(effect.render(context));

  bool anyDifference = false;
  for (std::size_t i = 0; i < pixels.size(); ++i) {
    if (pixels[i] != original[i]) {
      anyDifference = true;
      break;
    }
  }
  EXPECT_TRUE(anyDifference);
}
