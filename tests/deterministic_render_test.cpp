#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

namespace {

bool writeSineWav(const std::filesystem::path& path, int sampleRate, int frames) {
  constexpr int kChannels = 2;
  constexpr double kFrequency = 440.0;
  std::vector<int16_t> samples(static_cast<size_t>(frames) * kChannels);
  constexpr double kTwoPi = 6.28318530717958647692;
  for (int i = 0; i < frames; ++i) {
    double t = static_cast<double>(i) / static_cast<double>(sampleRate);
    const int16_t value = static_cast<int16_t>(std::sin(kTwoPi * kFrequency * t) * 32767.0);
    for (int c = 0; c < kChannels; ++c) {
      samples[static_cast<size_t>(i) * kChannels + c] = value;
    }
  }

  std::ofstream out(path, std::ios::binary);
  if (!out) {
    return false;
  }

  const uint32_t dataSize = static_cast<uint32_t>(samples.size() * sizeof(int16_t));
  const uint32_t chunkSize = 36u + dataSize;
  const uint16_t audioFormat = 1;
  const uint16_t numChannels = kChannels;
  const uint32_t sampleRate32 = static_cast<uint32_t>(sampleRate);
  const uint32_t byteRate = sampleRate32 * numChannels * sizeof(int16_t);
  const uint16_t blockAlign = numChannels * sizeof(int16_t);
  const uint16_t bitsPerSample = 16;

  out.write("RIFF", 4);
  out.write(reinterpret_cast<const char*>(&chunkSize), 4);
  out.write("WAVE", 4);
  out.write("fmt ", 4);
  const uint32_t subchunk1Size = 16;
  out.write(reinterpret_cast<const char*>(&subchunk1Size), 4);
  out.write(reinterpret_cast<const char*>(&audioFormat), 2);
  out.write(reinterpret_cast<const char*>(&numChannels), 2);
  out.write(reinterpret_cast<const char*>(&sampleRate32), 4);
  out.write(reinterpret_cast<const char*>(&byteRate), 4);
  out.write(reinterpret_cast<const char*>(&blockAlign), 2);
  out.write(reinterpret_cast<const char*>(&bitsPerSample), 2);
  out.write("data", 4);
  out.write(reinterpret_cast<const char*>(&dataSize), 4);
  out.write(reinterpret_cast<const char*>(samples.data()), dataSize);
  return out.good();
}

}  // namespace

TEST(DeterministicRender, Phase1PresetsMatchGolden) {
  namespace fs = std::filesystem;
  fs::path buildDir{BUILD_DIR};
  fs::path sourceDir{SOURCE_DIR};
  fs::path player = buildDir / "apps/avs-player/avs-player";
  fs::path wav = sourceDir / "tests/data/test.wav";
  fs::path phase1DataDir = sourceDir / "tests/data/phase1";
  fs::path phase1GoldenDir = sourceDir / "tests/golden/phase1";
  constexpr int kFrameCount = 10;

  ASSERT_TRUE(fs::exists(player)) << "Headless player missing at " << player;
  ASSERT_TRUE(fs::exists(phase1DataDir)) << "Phase1 preset directory missing at " << phase1DataDir;
  ASSERT_TRUE(fs::exists(phase1GoldenDir))
      << "Phase1 golden directory missing at " << phase1GoldenDir;

  auto quotePath = [](const fs::path& path) {
    std::ostringstream quoted;
    quoted << '"' << path.string() << '"';
    return quoted.str();
  };

  auto readLines = [](const fs::path& file) {
    std::ifstream in(file);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
      lines.push_back(line);
    }
    return lines;
  };

  auto readBinary = [](const fs::path& file) {
    std::ifstream in(file, std::ios::binary);
    return std::vector<char>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
  };

  auto listPngs = [](const fs::path& dir) {
    std::vector<std::string> files;
    for (const auto& entry : fs::directory_iterator(dir)) {
      if (entry.is_regular_file() && entry.path().extension() == ".png") {
        files.push_back(entry.path().filename().string());
      }
    }
    std::sort(files.begin(), files.end());
    return files;
  };

  std::vector<fs::path> presets;
  for (const auto& entry : fs::directory_iterator(phase1DataDir)) {
    if (entry.is_regular_file() && entry.path().extension() == ".avs") {
      presets.push_back(entry.path());
    }
  }
  std::sort(presets.begin(), presets.end());
  ASSERT_FALSE(presets.empty()) << "No presets found in " << phase1DataDir;

  fs::path runRoot = buildDir / "deterministic_phase1";
  fs::remove_all(runRoot);
  fs::create_directories(runRoot);

  for (const auto& preset : presets) {
    SCOPED_TRACE(preset.string());
    const std::string presetName = preset.stem().string();
    fs::path presetOut = runRoot / presetName;
    fs::remove_all(presetOut);
    fs::create_directories(presetOut);

    std::string cmd = quotePath(player) + " --headless --wav " + quotePath(wav) + " --preset " +
                      quotePath(preset) + " --frames " + std::to_string(kFrameCount) + " --out " +
                      quotePath(presetOut);
    int ret = std::system(cmd.c_str());
    ASSERT_EQ(ret, 0) << "Failed to render preset " << preset;

    fs::path outputHashes = presetOut / "hashes.txt";
    ASSERT_TRUE(fs::exists(outputHashes)) << "Missing hashes.txt for preset " << preset;

    fs::path goldenDir = phase1GoldenDir / presetName;
    ASSERT_TRUE(fs::exists(goldenDir)) << "Missing golden directory for preset " << presetName;

    fs::path goldenHashes = goldenDir / "hashes.txt";
    ASSERT_TRUE(fs::exists(goldenHashes)) << "Missing golden hashes for preset " << presetName;

    auto gotHashes = readLines(outputHashes);
    auto expectedHashes = readLines(goldenHashes);
    ASSERT_EQ(gotHashes.size(), expectedHashes.size())
        << "Hash count mismatch for preset " << presetName;
    for (size_t i = 0; i < expectedHashes.size(); ++i) {
      EXPECT_EQ(gotHashes[i], expectedHashes[i]) << "Hash mismatch on frame " << i;
    }

    auto expectedPngs = listPngs(goldenDir);
    auto gotPngs = listPngs(presetOut);
    ASSERT_EQ(gotPngs.size(), expectedPngs.size())
        << "PNG count mismatch for preset " << presetName;

    for (size_t i = 0; i < expectedPngs.size(); ++i) {
      fs::path gotFile = presetOut / gotPngs[i];
      fs::path expectedFile = goldenDir / expectedPngs[i];
      ASSERT_TRUE(fs::exists(expectedFile))
          << "Missing golden frame " << expectedPngs[i] << " for preset " << presetName;
      ASSERT_TRUE(fs::exists(gotFile))
          << "Missing rendered frame " << gotPngs[i] << " for preset " << presetName;
      auto gotBytes = readBinary(gotFile);
      auto expectedBytes = readBinary(expectedFile);
      ASSERT_FALSE(expectedBytes.empty())
          << "Golden frame " << expectedPngs[i] << " for preset " << presetName << " is empty";
      EXPECT_EQ(gotBytes, expectedBytes)
          << "PNG mismatch for frame " << expectedPngs[i] << " in preset " << presetName;
    }
  }

  fs::remove_all(runRoot);
}

TEST(DeterministicRender, WavRequiresHeadless) {
  namespace fs = std::filesystem;
  fs::path buildDir{BUILD_DIR};
  fs::path sourceDir{SOURCE_DIR};
  fs::path player = buildDir / "apps/avs-player/avs-player";
  fs::path wav = sourceDir / "tests/data/test.wav";
  fs::path preset = sourceDir / "tests/data/simple.avs";

  std::string cmd =
      player.string() + " --wav " + wav.string() + " --preset " + preset.string() + " --frames 60";
  int ret = std::system(cmd.c_str());
  EXPECT_NE(ret, 0);
}

TEST(DeterministicRender, HandlesGeneratedSampleRates) {
  namespace fs = std::filesystem;
  fs::path buildDir{BUILD_DIR};
  fs::path sourceDir{SOURCE_DIR};
  fs::path player = buildDir / "apps/avs-player/avs-player";
  fs::path preset = sourceDir / "tests/data/simple.avs";
  fs::path tempDir = buildDir / "sample_rate_runs";
  fs::remove_all(tempDir);
  fs::create_directories(tempDir);

  fs::path wav441 = tempDir / "sine44100.wav";
  fs::path wav480 = tempDir / "sine48000.wav";
  ASSERT_TRUE(writeSineWav(wav441, 44100, 4410));
  ASSERT_TRUE(writeSineWav(wav480, 48000, 4800));

  auto runHeadless = [&](const fs::path& wav, const fs::path& outDir) {
    fs::remove_all(outDir);
    fs::create_directories(outDir);
    std::string cmd = player.string() + " --headless --wav " + wav.string() + " --preset " +
                      preset.string() + " --frames 60 --out " + outDir.string();
    return std::system(cmd.c_str());
  };

  fs::path out441 = tempDir / "out441";
  fs::path out480 = tempDir / "out480";
  EXPECT_EQ(runHeadless(wav441, out441), 0);
  EXPECT_TRUE(fs::exists(out441 / "hashes.txt"));
  EXPECT_EQ(runHeadless(wav480, out480), 0);
  EXPECT_TRUE(fs::exists(out480 / "hashes.txt"));
}
