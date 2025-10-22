#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <avs/core/EffectRegistry.hpp>
#include <avs/core/Pipeline.hpp>
#include <avs/core/RenderContext.hpp>
#include <avs/effects/RegisterEffects.hpp>
#include <avs/offscreen/Md5.hpp>
#include <avs/effects/filters/effect_conv3x3.h>
#include <avs/effects/filters/effect_fast_brightness.h>
#include <avs/effects/trans/effect_water.h>
#include <avs/effects/trans/effect_color_reduction.h>


namespace {

constexpr int kWidth = 64;
constexpr int kHeight = 48;
constexpr int kFrames = 6;

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
      const int r = (x * 37 + y * 13) & 0xFF;
      const int g = (x * 11 + y * 59 + 17) & 0xFF;
      const int b = (x * 23 + y * 7 + 91) & 0xFF;
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

  avs::core::RenderContext context;
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
  return std::filesystem::path(SOURCE_DIR) / "tests" / "presets" / "filters";
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

class FilterEffectTests : public ::testing::Test {
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

TEST_F(FilterEffectTests, BlurBoxGolden) {
  avs::core::ParamBlock params;
  params.setInt("radius", 2);
  params.setBool("preserve_alpha", true);
  const auto result = renderEffect("filter_blur_box", params);
  expectGolden("blur_box", result.hashes);
}

TEST_F(FilterEffectTests, GrainGolden) {
  avs::core::ParamBlock params;
  params.setInt("amount", 28);
  params.setInt("seed", 77);
  params.setBool("static", false);
  params.setBool("monochrome", false);
  const auto result = renderEffect("filter_grain", params);
  expectGolden("grain", result.hashes);
}

TEST_F(FilterEffectTests, InterferencesGolden) {
  avs::core::ParamBlock params;
  params.setInt("amplitude", 96);
  params.setInt("period", 11);
  params.setInt("speed", 3);
  params.setInt("noise", 24);
  params.setInt("phase", 4);
  params.setInt("tint", 0x80FF40);
  params.setString("mode", "add");
  params.setBool("vertical", false);
  const auto result = renderEffect("filter_interferences", params);
  expectGolden("interferences", result.hashes);
}

TEST_F(FilterEffectTests, FastBrightnessGolden) {
  avs::core::ParamBlock params;
  params.setFloat("amount", 1.6f);
  params.setFloat("bias", 4.0f);
  params.setBool("clamp", true);
  const auto result = renderEffect("filter_fast_brightness", params);
  expectGolden("fast_brightness", result.hashes);
}

TEST(FastBrightnessEffect, HonorsClampOutputModes) {
  {
    avs::effects::filters::FastBrightness effect;
    avs::core::ParamBlock params;
    params.setFloat("amount", 1.5f);
    params.setFloat("bias", 200.0f);
    params.setBool("clamp", true);
    effect.setParams(params);

    std::array<std::uint8_t, 4> pixel{200u, 10u, 180u, 255u};
    avs::core::RenderContext context{};
    context.width = 1;
    context.height = 1;
    context.framebuffer = {pixel.data(), pixel.size()};

    ASSERT_TRUE(effect.render(context));
    EXPECT_EQ(pixel[0], 255u);
    EXPECT_EQ(pixel[1], 215u);
    EXPECT_EQ(pixel[2], 255u);
    EXPECT_EQ(pixel[3], 255u);
  }

  {
    avs::effects::filters::FastBrightness effect;
    avs::core::ParamBlock params;
    params.setFloat("amount", 1.5f);
    params.setFloat("bias", 200.0f);
    params.setBool("clamp", false);
    effect.setParams(params);

    std::array<std::uint8_t, 4> pixel{200u, 10u, 180u, 255u};
    avs::core::RenderContext context{};
    context.width = 1;
    context.height = 1;
    context.framebuffer = {pixel.data(), pixel.size()};

    ASSERT_TRUE(effect.render(context));
    EXPECT_EQ(pixel[0], static_cast<std::uint8_t>(500));
    EXPECT_EQ(pixel[1], 215u);
    EXPECT_EQ(pixel[2], static_cast<std::uint8_t>(470));
    EXPECT_EQ(pixel[3], 255u);
  }

  {
    avs::effects::filters::FastBrightness effect;
    avs::core::ParamBlock params;
    params.setFloat("amount", 1.0f);
    params.setFloat("bias", -300.0f);
    params.setBool("clamp", true);
    effect.setParams(params);

    std::array<std::uint8_t, 4> pixel{50u, 5u, 123u, 255u};
    avs::core::RenderContext context{};
    context.width = 1;
    context.height = 1;
    context.framebuffer = {pixel.data(), pixel.size()};

    ASSERT_TRUE(effect.render(context));
    EXPECT_EQ(pixel[0], 0u);
    EXPECT_EQ(pixel[1], 0u);
    EXPECT_EQ(pixel[2], 0u);
    EXPECT_EQ(pixel[3], 255u);
  }

  {
    avs::effects::filters::FastBrightness effect;
    avs::core::ParamBlock params;
    params.setFloat("amount", 1.0f);
    params.setFloat("bias", -300.0f);
    params.setBool("clamp", false);
    effect.setParams(params);

    std::array<std::uint8_t, 4> pixel{50u, 5u, 123u, 255u};
    avs::core::RenderContext context{};
    context.width = 1;
    context.height = 1;
    context.framebuffer = {pixel.data(), pixel.size()};

    ASSERT_TRUE(effect.render(context));
    EXPECT_EQ(pixel[0], static_cast<std::uint8_t>(-250));
    EXPECT_EQ(pixel[1], static_cast<std::uint8_t>(-295));
    EXPECT_EQ(pixel[2], static_cast<std::uint8_t>(-177));
    EXPECT_EQ(pixel[3], 255u);
  }
}

TEST(Convolution3x3Effect, HonorsClampOutputModes) {
  const std::string kernel = "0 0 0 0 2 0 0 0 0";

  {
    avs::effects::filters::Convolution3x3 effect;
    avs::core::ParamBlock params;
    params.setString("kernel", kernel);
    params.setFloat("divisor", 1.0f);
    params.setFloat("bias", 0.0f);
    params.setBool("preserve_alpha", false);
    params.setBool("clamp", true);
    effect.setParams(params);

    std::array<std::uint8_t, 4> pixel{200u, 10u, 180u, 200u};
    avs::core::RenderContext context{};
    context.width = 1;
    context.height = 1;
    context.framebuffer = {pixel.data(), pixel.size()};

    ASSERT_TRUE(effect.render(context));
    EXPECT_EQ(pixel[0], 255u);
    EXPECT_EQ(pixel[1], 20u);
    EXPECT_EQ(pixel[2], 255u);
    EXPECT_EQ(pixel[3], 255u);
  }

  {
    avs::effects::filters::Convolution3x3 effect;
    avs::core::ParamBlock params;
    params.setString("kernel", kernel);
    params.setFloat("divisor", 1.0f);
    params.setFloat("bias", 0.0f);
    params.setBool("preserve_alpha", false);
    params.setBool("clamp", false);
    effect.setParams(params);

    std::array<std::uint8_t, 4> pixel{200u, 10u, 180u, 200u};
    avs::core::RenderContext context{};
    context.width = 1;
    context.height = 1;
    context.framebuffer = {pixel.data(), pixel.size()};

    ASSERT_TRUE(effect.render(context));
    EXPECT_EQ(pixel[0], static_cast<std::uint8_t>(400));
    EXPECT_EQ(pixel[1], static_cast<std::uint8_t>(20));
    EXPECT_EQ(pixel[2], static_cast<std::uint8_t>(360));
    EXPECT_EQ(pixel[3], static_cast<std::uint8_t>(400));
  }
}

TEST_F(FilterEffectTests, ColorMapGolden) {
  std::ostringstream table;
  table << std::hex << std::setfill('0');
  for (int i = 0; i < 256; ++i) {
    const int r = (i * 5) & 0xFF;
    const int g = (255 - i) & 0xFF;
    const int b = (i * 9 + 17) & 0xFF;
    table << std::setw(2) << r << std::setw(2) << g << std::setw(2) << b;
    if (i + 1 != 256) {
      table << ' ';
    }
  }

  avs::core::ParamBlock params;
  params.setString("table", table.str());
  params.setString("channel", "luma");
  params.setBool("map_alpha", false);
  const auto result = renderEffect("filter_color_map", params);
  expectGolden("color_map", result.hashes);
}

TEST_F(FilterEffectTests, Conv3x3Golden) {
  avs::core::ParamBlock params;
  params.setString("kernel", "0 -1 0 -1 5 -1 0 -1 0");
  params.setFloat("bias", 0.0f);
  params.setFloat("divisor", 1.0f);
  params.setBool("preserve_alpha", true);
  params.setBool("clamp", true);
  const auto result = renderEffect("filter_conv3x3", params);
  expectGolden("conv3x3", result.hashes);
}

TEST(WaterEffect, SimulatesIntegerRipplePropagation) {
  avs::effects::trans::Water effect;

  std::array<std::uint8_t, 3 * 3 * 4> pixels{};
  auto setPixel = [&](int x, int y, std::uint8_t r) {
    const std::size_t index = (static_cast<std::size_t>(y) * 3 + static_cast<std::size_t>(x)) * 4u;
    pixels[index + 0] = r;
    pixels[index + 1] = 0u;
    pixels[index + 2] = 0u;
    pixels[index + 3] = 255u;
  };

  setPixel(0, 0, 0u);
  setPixel(1, 0, 64u);
  setPixel(2, 0, 128u);
  setPixel(0, 1, 32u);
  setPixel(1, 1, 96u);
  setPixel(2, 1, 160u);
  setPixel(0, 2, 64u);
  setPixel(1, 2, 128u);
  setPixel(2, 2, 192u);

  avs::core::RenderContext context{};
  context.width = 3;
  context.height = 3;
  context.framebuffer = {pixels.data(), pixels.size()};

  ASSERT_TRUE(effect.render(context));

  const std::array<std::uint8_t, 9> expectedFirstFrame = {96u,  112u, 224u, 80u, 192u,
                                                          208u, 160u, 176u, 255u};
  for (int y = 0; y < 3; ++y) {
    for (int x = 0; x < 3; ++x) {
      const std::size_t index =
          (static_cast<std::size_t>(y) * 3 + static_cast<std::size_t>(x)) * 4u;
      const std::size_t flat = static_cast<std::size_t>(y) * 3u + static_cast<std::size_t>(x);
      EXPECT_EQ(pixels[index + 0], expectedFirstFrame[flat])
          << "Mismatch at (" << x << ", " << y << ")";
      EXPECT_EQ(pixels[index + 1], 0u);
      EXPECT_EQ(pixels[index + 2], 0u);
      EXPECT_EQ(pixels[index + 3], 255u);
    }
  }

  for (std::size_t i = 0; i < pixels.size(); i += 4) {
    pixels[i + 0] = 0u;
    pixels[i + 1] = 0u;
    pixels[i + 2] = 0u;
    pixels[i + 3] = 255u;
  }

  ASSERT_TRUE(effect.render(context));

  for (std::size_t i = 0; i < pixels.size(); i += 4) {
    EXPECT_EQ(pixels[i + 0], 0u);
    EXPECT_EQ(pixels[i + 1], 0u);
    EXPECT_EQ(pixels[i + 2], 0u);
    EXPECT_EQ(pixels[i + 3], 255u);
  }
}

TEST(ColorReductionEffect, MasksRgbChannelsToRequestedDepth) {
  avs::effects::trans::ColorReduction effect;
  avs::core::ParamBlock params;
  params.setInt("levels", 5);
  effect.setParams(params);

  std::array<std::uint8_t, 4> pixel{0b10110110u, 0b01101101u, 0b11110000u, 0xAAu};
  avs::core::RenderContext context{};
  context.width = 1;
  context.height = 1;
  context.framebuffer = {pixel.data(), pixel.size()};

  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(pixel[0], 0b10110000u);
  EXPECT_EQ(pixel[1], 0b01101000u);
  EXPECT_EQ(pixel[2], 0b11110000u);
  EXPECT_EQ(pixel[3], 0xAAu);
}

TEST(ColorReductionEffect, ClampsLevelsAndSupportsBitAlias) {
  avs::effects::trans::ColorReduction effect;
  avs::core::ParamBlock params;
  params.setInt("levels", 9);  // clamps to 8
  effect.setParams(params);

  std::array<std::uint8_t, 4> pixel{12u, 34u, 56u, 78u};
  avs::core::RenderContext context{};
  context.width = 1;
  context.height = 1;
  context.framebuffer = {pixel.data(), pixel.size()};

  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(pixel[0], 12u);
  EXPECT_EQ(pixel[1], 34u);
  EXPECT_EQ(pixel[2], 56u);
  EXPECT_EQ(pixel[3], 78u);

  pixel = {200u, 150u, 90u, 78u};
  context.framebuffer = {pixel.data(), pixel.size()};
  params = avs::core::ParamBlock{};
  params.setInt("bits", 2);
  effect.setParams(params);

  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(pixel[0], 192u);
  EXPECT_EQ(pixel[1], 128u);
  EXPECT_EQ(pixel[2], 64u);
  EXPECT_EQ(pixel[3], 78u);
}


