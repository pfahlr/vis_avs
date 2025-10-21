#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <string_view>
#include <vector>

#include "avs/core/EffectRegistry.hpp"
#include "avs/core/ParamBlock.hpp"
#include "avs/core/Pipeline.hpp"
#include "avs/core/RenderContext.hpp"
#include "avs/effects/RegisterEffects.hpp"
#include "avs/offscreen/Md5.hpp"
#include "effects/trans/effect_blur.h"

namespace {

constexpr int kWidth = 64;
constexpr int kHeight = 48;
constexpr int kFrames = 6;

struct FrameHashResult {
  std::vector<std::uint8_t> pixels;
  std::vector<std::string> hashes;
};

std::vector<std::uint8_t> makeBasePattern() {
  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(kWidth) * static_cast<std::size_t>(kHeight) * 4u);
  for (int y = 0; y < kHeight; ++y) {
    for (int x = 0; x < kWidth; ++x) {
      const std::size_t index = (static_cast<std::size_t>(y) * kWidth + static_cast<std::size_t>(x)) * 4u;
      const int r = (x * 29 + y * 17) & 0xFF;
      const int g = (x * 41 + y * 11 + 67) & 0xFF;
      const int b = (x * 7 + y * 53 + 19) & 0xFF;
      pixels[index + 0] = static_cast<std::uint8_t>(r);
      pixels[index + 1] = static_cast<std::uint8_t>(g);
      pixels[index + 2] = static_cast<std::uint8_t>(b);
      pixels[index + 3] = 255u;
    }
  }
  return pixels;
}

FrameHashResult renderEffect(const std::string& effectKey, const avs::core::ParamBlock& params) {
  avs::core::EffectRegistry registry;
  avs::effects::registerCoreEffects(registry);

  avs::core::Pipeline pipeline(registry);
  pipeline.add(effectKey, params);

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
    std::copy(result.pixels.begin(), result.pixels.end(), working.begin());
    pipeline.render(context);
    result.hashes.push_back(avs::offscreen::computeMd5Hex(working.data(), working.size()));
  }

  std::swap(result.pixels, working);
  return result;
}

std::filesystem::path goldenDir() {
  return std::filesystem::path(SOURCE_DIR) / "tests" / "presets" / "trans";
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

void expectGolden(const std::string& effectName, const std::vector<std::string>& hashes) {
  const auto path = goldenDir() / effectName / "hashes.md5";
  const auto golden = loadGolden(path);
  if (golden.empty()) {
    ADD_FAILURE() << "Missing golden hashes for " << effectName << " at " << path
                  << "\nCaptured hashes:\n";
    for (const auto& hash : hashes) {
      ADD_FAILURE() << hash;
    }
    return;
  }
  ASSERT_EQ(golden.size(), hashes.size()) << "Golden/hash count mismatch for " << effectName;
  EXPECT_EQ(golden, hashes) << "Golden mismatch for " << effectName;
}

class TransEffectTests : public ::testing::Test {
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

TEST_F(TransEffectTests, BlurFullGolden) {
  avs::core::ParamBlock params;
  params.setInt("radius", 2);
  params.setInt("strength", 256);
  params.setBool("horizontal", true);
  params.setBool("vertical", true);

  const auto result = renderEffect("trans / blur", params);
  expectGolden("blur_box", result.hashes);
}

TEST_F(TransEffectTests, BlurHorizontalIsolationGolden) {
  avs::core::ParamBlock params;
  params.setInt("radius", 2);
  params.setInt("strength", 256);
  params.setBool("horizontal", true);
  params.setBool("vertical", false);

  const auto result = renderEffect("trans / blur", params);
  expectGolden("blur_box_horizontal", result.hashes);
}

TEST(TransBlurEffect, StrengthBlendingRespectsHalfMix) {
  avs::effects::trans::R_Blur effect;
  avs::core::ParamBlock params;
  params.setInt("radius", 1);
  params.setInt("strength", 128);
  params.setBool("horizontal", true);
  params.setBool("vertical", false);
  effect.setParams(params);

  std::array<std::uint8_t, 12> pixels{0u, 0u, 0u, 255u, 100u, 150u, 200u, 255u, 200u, 50u, 0u, 255u};
  avs::core::RenderContext context{};
  context.width = 3;
  context.height = 1;
  context.framebuffer = {pixels.data(), pixels.size()};

  ASSERT_TRUE(effect.render(context));
  const std::array<std::uint8_t, 12> expected{16u, 25u, 33u, 255u, 100u, 108u, 133u, 255u, 183u, 66u, 33u, 255u};
  EXPECT_EQ(pixels, expected);
}

TEST(TransBlurEffect, LegacyEnabledMappingMatchesExplicitRadiusAndStrength) {
  avs::core::ParamBlock legacyParams;
  legacyParams.setInt("enabled", 2);

  avs::core::ParamBlock explicitParams;
  explicitParams.setInt("radius", 1);
  explicitParams.setInt("strength", 192);

  const auto legacy = renderEffect("trans / blur", legacyParams);
  const auto explicitControl = renderEffect("trans / blur", explicitParams);

  EXPECT_EQ(legacy.hashes, explicitControl.hashes);
  EXPECT_EQ(legacy.pixels, explicitControl.pixels);
}
