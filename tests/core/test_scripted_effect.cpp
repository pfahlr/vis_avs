#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <avs/core/EffectRegistry.hpp>
#include <avs/core/ParamBlock.hpp>
#include <avs/core/Pipeline.hpp>
#include <avs/core/RenderContext.hpp>
#include <avs/effects/prime/RegisterEffects.hpp>
#include <avs/effects/prime/micro_preset_parser.hpp>
#include <avs/offscreen/Md5.hpp>

namespace {

class ScriptedGoldenStore {
 public:
  ScriptedGoldenStore() {
    path_ = std::filesystem::path(SOURCE_DIR) / "tests/golden/scripted_md5.txt";
    load();
    updateMode_ = std::getenv("UPDATE_GOLDENS") != nullptr;
  }

  void expect(const std::string& key, const std::vector<std::string>& values) {
    if (updateMode_) {
      entries_[key] = values;
      write();
      return;
    }
    auto it = entries_.find(key);
    ASSERT_NE(it, entries_.end()) << "Missing scripted golden for " << key;
    EXPECT_EQ(it->second, values);
  }

 private:
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
      if (!(iss >> name)) {
        continue;
      }
      std::vector<std::string> hashes;
      std::string hash;
      while (iss >> hash) {
        hashes.push_back(hash);
      }
      if (!hashes.empty()) {
        entries_[name] = hashes;
      }
    }
  }

  void write() const {
    std::vector<std::pair<std::string, std::vector<std::string>>> sorted(entries_.begin(), entries_.end());
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    std::ofstream file(path_, std::ios::trunc);
    if (!file) {
      throw std::runtime_error("failed to write scripted golden: " + path_.string());
    }
    file << "# preset hash1 hash2 ...\n";
    for (const auto& [name, hashes] : sorted) {
      file << name;
      for (const auto& h : hashes) {
        file << ' ' << h;
      }
      file << '\n';
    }
  }

  std::filesystem::path path_;
  std::unordered_map<std::string, std::vector<std::string>> entries_;
  bool updateMode_ = false;
};

std::string loadFile(const std::filesystem::path& path) {
  std::ifstream file(path);
  if (!file) {
    throw std::runtime_error("failed to open scripted preset: " + path.string());
  }
  std::ostringstream oss;
  oss << file.rdbuf();
  return oss.str();
}

std::vector<std::string> renderPreset(const std::filesystem::path& presetPath,
                                      avs::core::EffectRegistry& registry,
                                      int width,
                                      int height,
                                      int frames) {
  const std::string text = loadFile(presetPath);
  auto parsed = avs::effects::parseMicroPreset(text);

  avs::core::Pipeline pipeline(registry);
  for (const auto& cmd : parsed.commands) {
    pipeline.add(cmd.effectKey, cmd.params);
  }

  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u, 0);
  std::vector<float> spectrum(96, 0.0f);

  avs::core::RenderContext ctx;
  ctx.width = width;
  ctx.height = height;
  ctx.deltaSeconds = 1.0 / 60.0;
  ctx.framebuffer = {pixels.data(), pixels.size()};
  ctx.audioSpectrum = {spectrum.data(), spectrum.size()};

  std::vector<std::string> hashes;
  hashes.reserve(static_cast<std::size_t>(frames));

  for (int frame = 0; frame < frames; ++frame) {
    ctx.frameIndex = static_cast<std::uint64_t>(frame);
    ctx.rng.reseed(ctx.frameIndex);
    if (!pipeline.render(ctx)) {
      throw std::runtime_error("scripted pipeline render failed");
    }
    hashes.push_back(avs::offscreen::computeMd5Hex(pixels.data(), pixels.size()));
  }

  return hashes;
}

class ScriptedEffectGoldenTest : public ::testing::Test {
 protected:
  ScriptedEffectGoldenTest() { avs::effects::registerCoreEffects(registry_); }

  avs::core::EffectRegistry registry_;
};

TEST_F(ScriptedEffectGoldenTest, PresetsMatchGolden) {
  namespace fs = std::filesystem;
  fs::path root = fs::path(SOURCE_DIR) / "tests/presets/scripted";
  ASSERT_TRUE(fs::exists(root)) << "Missing scripted preset directory at " << root;

  std::vector<fs::path> files;
  for (const auto& entry : fs::directory_iterator(root)) {
    if (entry.is_regular_file() && entry.path().extension() == ".micro") {
      files.push_back(entry.path());
    }
  }
  std::sort(files.begin(), files.end());
  ASSERT_FALSE(files.empty()) << "No scripted presets found in " << root;

  ScriptedGoldenStore golden;
  constexpr int kWidth = 64;
  constexpr int kHeight = 64;
  constexpr int kFrameCount = 4;

  for (const auto& file : files) {
    SCOPED_TRACE(file.string());
    auto hashes = renderPreset(file, registry_, kWidth, kHeight, kFrameCount);
    golden.expect(file.stem().string(), hashes);
  }
}

}  // namespace

