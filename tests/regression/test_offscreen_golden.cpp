#include <gtest/gtest.h>

#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <avs/offscreen/OffscreenRenderer.hpp>
#include <avs/offscreen/Md5.hpp>

namespace {

struct ExpectedHashes {
  int width = 0;
  int height = 0;
  int seed = 0;
  std::vector<std::string> md5;
};

ExpectedHashes parseExpectedJson(const std::filesystem::path& path) {
  std::ifstream in(path);
  if (!in) {
    throw std::runtime_error("failed to open expected MD5 file: " + path.string());
  }
  std::ostringstream oss;
  oss << in.rdbuf();
  const std::string content = oss.str();

  auto findInt = [&](const std::string& key) -> int {
    const std::string needle = '"' + key + '"';
    const auto keyPos = content.find(needle);
    if (keyPos == std::string::npos) {
      throw std::runtime_error("missing key in expected JSON: " + key);
    }
    const auto colonPos = content.find(':', keyPos);
    if (colonPos == std::string::npos) {
      throw std::runtime_error("missing colon for key: " + key);
    }
    size_t valuePos = colonPos + 1;
    while (valuePos < content.size() && std::isspace(static_cast<unsigned char>(content[valuePos]))) {
      ++valuePos;
    }
    size_t endPos = valuePos;
    while (endPos < content.size() && (std::isdigit(static_cast<unsigned char>(content[endPos])) ||
                                       content[endPos] == '+' || content[endPos] == '-')) {
      ++endPos;
    }
    return std::stoi(content.substr(valuePos, endPos - valuePos));
  };

  ExpectedHashes expected;
  expected.width = findInt("width");
  expected.height = findInt("height");
  expected.seed = findInt("seed");

  const std::string md5Key = "\"md5\"";
  auto md5Pos = content.find(md5Key);
  if (md5Pos == std::string::npos) {
    throw std::runtime_error("missing md5 key in expected JSON");
  }
  auto arrayStart = content.find('[', md5Pos);
  auto arrayEnd = content.find(']', md5Pos);
  if (arrayStart == std::string::npos || arrayEnd == std::string::npos || arrayEnd <= arrayStart) {
    throw std::runtime_error("invalid md5 array in expected JSON");
  }
  std::string arrayBody = content.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
  std::stringstream ss(arrayBody);
  std::string token;
  while (std::getline(ss, token, '"')) {
    if (token.size() == 32 && token.find_first_not_of("0123456789abcdefABCDEF") == std::string::npos) {
      expected.md5.push_back(token);
    }
  }
  return expected;
}

std::vector<float> generateAudioBuffer(unsigned sampleRate, unsigned channels, double silenceSeconds,
                                       double toneSeconds, double frequencyHz) {
  const size_t silenceFrames = static_cast<size_t>(silenceSeconds * static_cast<double>(sampleRate));
  const size_t toneFrames = static_cast<size_t>(toneSeconds * static_cast<double>(sampleRate));
  const size_t totalFrames = silenceFrames + toneFrames;
  std::vector<float> samples(totalFrames * static_cast<size_t>(channels), 0.0f);
  const double twoPiF = 2.0 * 3.14159265358979323846 * frequencyHz;
  for (size_t frame = silenceFrames; frame < totalFrames; ++frame) {
    double t = static_cast<double>(frame - silenceFrames) / static_cast<double>(sampleRate);
    float value = static_cast<float>(std::sin(twoPiF * t));
    for (unsigned ch = 0; ch < channels; ++ch) {
      samples[frame * static_cast<size_t>(channels) + ch] = value;
    }
  }
  return samples;
}

}  // namespace

TEST(OffscreenGoldenTest, FramesMatchExpectedMd5) {
#ifdef _WIN32
  _putenv_s("AVS_SEED", "1234");
#else
  ::setenv("AVS_SEED", "1234", 1);
#endif

  const std::filesystem::path expectedPath =
      std::filesystem::path(SOURCE_DIR) /
      "tests/regression/data/expected_md5_320x240_seed1234.json";
  const ExpectedHashes expected = parseExpectedJson(expectedPath);

  ASSERT_EQ(expected.width, 320);
  ASSERT_EQ(expected.height, 240);
  ASSERT_EQ(expected.seed, 1234);
  ASSERT_EQ(expected.md5.size(), 10u);

  avs::offscreen::OffscreenRenderer renderer(expected.width, expected.height);

  const std::filesystem::path presetPath =
      std::filesystem::path(SOURCE_DIR) /
      "tests/regression/data/tiny_preset_fragment.avs";
  ASSERT_NO_THROW(renderer.loadPreset(presetPath));

  auto audio = generateAudioBuffer(48000, 2, 0.05, 0.5, 1000.0);
  renderer.setAudioBuffer(std::move(audio), 48000, 2);

  std::vector<std::string> md5Values;
  md5Values.reserve(expected.md5.size());

  for (size_t i = 0; i < expected.md5.size(); ++i) {
    auto frame = renderer.render();
    ASSERT_EQ(frame.width, expected.width);
    ASSERT_EQ(frame.height, expected.height);
    ASSERT_EQ(frame.size, static_cast<size_t>(expected.width * expected.height * 4));
    md5Values.push_back(avs::offscreen::computeMd5Hex(frame.data, frame.size));
  }

  EXPECT_EQ(md5Values, expected.md5);
}

