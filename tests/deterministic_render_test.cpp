#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

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
