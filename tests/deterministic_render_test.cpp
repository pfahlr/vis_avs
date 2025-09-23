#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
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

TEST(DeterministicRender, MatchesGolden) {
  namespace fs = std::filesystem;
  fs::path buildDir{BUILD_DIR};
  fs::path sourceDir{SOURCE_DIR};
  fs::path player = buildDir / "apps/avs-player/avs-player";
  fs::path wav = sourceDir / "tests/data/test.wav";
  fs::path preset = sourceDir / "tests/data/simple.avs";
  fs::path out = buildDir / "deterministic_out";
  fs::remove_all(out);
  fs::create_directories(out);

  std::string cmd = player.string() + " --headless --wav " + wav.string() + " --preset " +
                    preset.string() + " --frames 120 --out " + out.string();
  int ret = std::system(cmd.c_str());
  ASSERT_EQ(ret, 0);

  std::ifstream got(out / "hashes.txt");
  std::ifstream expected(sourceDir / "tests/golden/hashes.txt");
  ASSERT_TRUE(got);
  ASSERT_TRUE(expected);
  std::string gLine, eLine;
  while (std::getline(got, gLine) && std::getline(expected, eLine)) {
    EXPECT_EQ(gLine, eLine);
  }
  EXPECT_FALSE(std::getline(got, gLine));
  EXPECT_FALSE(std::getline(expected, eLine));
}

TEST(DeterministicRender, InteractiveWavPlaybackUsesOfflineAudio) {
  namespace fs = std::filesystem;
  fs::path buildDir{BUILD_DIR};
  fs::path sourceDir{SOURCE_DIR};
  fs::path player = buildDir / "apps/avs-player/avs-player";
  fs::path wav = sourceDir / "tests/data/test.wav";
  fs::path preset = sourceDir / "tests/data/simple.avs";

#if defined(_WIN32)
  _putenv("SDL_AUDIODRIVER=dummy");
#else
  setenv("SDL_VIDEODRIVER", "offscreen", 1);
  setenv("SDL_AUDIODRIVER", "dummy", 1);
#endif

  std::string cmd =
      player.string() + " --wav " + wav.string() + " --preset " + preset.string() + " --frames 10";
  int ret = std::system(cmd.c_str());
  EXPECT_EQ(ret, 0);
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
