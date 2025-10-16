#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "avs/core/EffectRegistry.hpp"
#include "avs/core/ParamBlock.hpp"
#include "avs/core/Pipeline.hpp"
#include "avs/core/RenderContext.hpp"
#include "avs/effects/RegisterEffects.hpp"
#include "avs/effects/blend_ops.hpp"
#include "avs/effects/micro_preset_parser.hpp"
#include "avs/offscreen/Md5.hpp"

namespace {

using avs::core::ParamBlock;
using avs::core::RenderContext;
using avs::effects::BlendOp;

std::string loadFile(const std::filesystem::path& path) {
  std::ifstream file(path);
  if (!file) {
    throw std::runtime_error("failed to open file: " + path.string());
  }
  std::ostringstream oss;
  oss << file.rdbuf();
  return oss.str();
}

RenderContext makeContext(std::vector<std::uint8_t>& pixels, int width, int height) {
  RenderContext ctx;
  ctx.frameIndex = 0;
  ctx.deltaSeconds = 1.0 / 60.0;
  ctx.width = width;
  ctx.height = height;
  ctx.framebuffer = {pixels.data(), pixels.size()};
  ctx.audioSpectrum = {nullptr, 0};
  return ctx;
}

class GoldenManager {
 public:
  static GoldenManager& instance() {
    static GoldenManager manager;
    return manager;
  }

  std::string expected(const std::string& key) const {
    auto it = md5_.find(key);
    if (it == md5_.end()) {
      return {};
    }
    return it->second;
  }

  void set(const std::string& key, const std::string& value) {
    md5_[key] = value;
    if (updateMode_) {
      write();
    }
  }

  bool updateMode() const { return updateMode_; }

 private:
  GoldenManager() {
    path_ = std::filesystem::path(SOURCE_DIR) / "tests/golden/micro_blend_md5.txt";
    load();
    updateMode_ = std::getenv("UPDATE_GOLDENS") != nullptr;
  }

  void load() {
    std::ifstream file(path_);
    if (!file) {
      return;
    }
    std::string line;
    while (std::getline(file, line)) {
      auto hashPos = line.find('#');
      if (hashPos != std::string::npos) {
        line = line.substr(0, hashPos);
      }
      std::istringstream iss(line);
      std::string name;
      std::string md5;
      if (!(iss >> name >> md5)) {
        continue;
      }
      md5_[name] = md5;
    }
  }

  void write() const {
    std::vector<std::pair<std::string, std::string>> entries(md5_.begin(), md5_.end());
    std::sort(entries.begin(), entries.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });
    std::ofstream file(path_, std::ios::trunc);
    if (!file) {
      throw std::runtime_error("failed to write golden file: " + path_.string());
    }
    file << "# name md5\n";
    for (const auto& [name, md5] : entries) {
      file << name << ' ' << md5 << "\n";
    }
  }

  std::unordered_map<std::string, std::string> md5_;
  std::filesystem::path path_;
  bool updateMode_ = false;
};

void expectGolden(const std::string& key, const std::string& value) {
  auto& golden = GoldenManager::instance();
  if (golden.updateMode()) {
    golden.set(key, value);
  } else {
    const auto expected = golden.expected(key);
    ASSERT_FALSE(expected.empty()) << "Missing golden entry for " << key;
    EXPECT_EQ(value, expected);
  }
}

std::string renderMicroPreset(const std::string& key, const std::filesystem::path& presetPath,
                              avs::core::EffectRegistry& registry,
                              const std::function<void(std::vector<std::uint8_t>&)>& initializer) {
  (void)key;
  const std::string presetText = loadFile(presetPath);
  const auto parsed = avs::effects::parseMicroPreset(presetText);

  avs::core::Pipeline pipeline(registry);
  for (const auto& command : parsed.commands) {
    pipeline.add(command.effectKey, command.params);
  }

  constexpr int kWidth = 2;
  constexpr int kHeight = 2;
  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(kWidth * kHeight * 4), 0);
  initializer(pixels);
  auto ctx = makeContext(pixels, kWidth, kHeight);
  EXPECT_TRUE(pipeline.render(ctx));
  return avs::offscreen::computeMd5Hex(pixels.data(), pixels.size());
}

std::string renderMicroPreset(const std::string& key, const std::filesystem::path& presetPath,
                              avs::core::EffectRegistry& registry) {
  return renderMicroPreset(key, presetPath, registry, [](std::vector<std::uint8_t>&) {});
}

}  // namespace

TEST(BlendOpParser, ParsesBlendTokens) {
  using avs::effects::parseBlendOpToken;
  auto add = parseBlendOpToken("additive");
  ASSERT_TRUE(add.has_value());
  EXPECT_EQ(*add, BlendOp::Additive);

  auto alpha2 = parseBlendOpToken("ALPHA2");
  ASSERT_TRUE(alpha2.has_value());
  EXPECT_EQ(*alpha2, BlendOp::Alpha2);

  auto slide = parseBlendOpToken("BlendSlide");
  ASSERT_TRUE(slide.has_value());
  EXPECT_EQ(*slide, BlendOp::BlendSlide);

  auto unknown = parseBlendOpToken("mystery");
  EXPECT_FALSE(unknown.has_value());
}

TEST(MicroPresetParser, HandlesParametersAndUiTokens) {
  const char* text = R"(BUTTON1 ignored
blend op=alpha alpha=200 slide=42 fg=0x00ff00
CHECKBOX extra
)";
  auto parsed = avs::effects::parseMicroPreset(text);
  ASSERT_EQ(parsed.commands.size(), 1u);
  const auto& cmd = parsed.commands.front();
  EXPECT_EQ(cmd.effectKey, "blend");
  EXPECT_EQ(cmd.params.getInt("alpha", 0), 200);
  EXPECT_EQ(cmd.params.getInt("slide", 0), 42);
  EXPECT_EQ(cmd.params.getInt("fg", 0), 0x00ff00);
  ASSERT_FALSE(parsed.warnings.empty());
}

class BlendEffectsTest : public ::testing::Test {
 protected:
  BlendEffectsTest() { avs::effects::registerCoreEffects(registry_); }

  avs::core::EffectRegistry registry_;
};

TEST_F(BlendEffectsTest, BlendAdditiveGolden) {
  const auto md5 = renderMicroPreset(
      "blend_additive",
      std::filesystem::path(SOURCE_DIR) / "tests/data/micro_presets/blend_additive.txt", registry_);
  expectGolden("blend_additive", md5);
}

TEST_F(BlendEffectsTest, BlendAlphaGolden) {
  const auto md5 = renderMicroPreset(
      "blend_alpha", std::filesystem::path(SOURCE_DIR) / "tests/data/micro_presets/blend_alpha.txt",
      registry_);
  expectGolden("blend_alpha", md5);
}

TEST_F(BlendEffectsTest, OverlayBlendSlideGolden) {
  const auto md5 = renderMicroPreset(
      "overlay_blendslide",
      std::filesystem::path(SOURCE_DIR) / "tests/data/micro_presets/overlay_blendslide.txt",
      registry_);
  expectGolden("overlay_blendslide", md5);
}

TEST_F(BlendEffectsTest, SwizzleBgrGolden) {
  const auto md5 = renderMicroPreset(
      "swizzle_bgr", std::filesystem::path(SOURCE_DIR) / "tests/data/micro_presets/swizzle_bgr.txt",
      registry_, [](std::vector<std::uint8_t>& pixels) {
        const std::array<std::array<std::uint8_t, 4>, 4> gradient{{
            {{0, 64, 128, 255}},
            {{32, 96, 160, 255}},
            {{64, 128, 192, 255}},
            {{96, 160, 224, 255}},
        }};
        for (size_t i = 0; i < gradient.size(); ++i) {
          std::copy(gradient[i].begin(), gradient[i].end(),
                    pixels.begin() + static_cast<std::ptrdiff_t>(i * 4));
        }
      });
  expectGolden("swizzle_bgr", md5);
}
