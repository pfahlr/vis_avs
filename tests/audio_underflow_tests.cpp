#include <gtest/gtest.h>

#include <array>
#include <vector>

#include "avs/audio_portaudio_internal.hpp"

namespace {

TEST(ProcessCallbackInputTest, WritesSamplesAndAdvancesIndex) {
  std::vector<float> ring(8, -1.0f);
  const size_t mask = ring.size() - 1;
  const size_t writeIndex = 2;
  const std::array<float, 4> input{1.0f, -0.5f, 0.25f, 0.75f};

  auto result = avs::portaudio_detail::processCallbackInput(input.data(), input.size(), writeIndex,
                                                            mask, ring);

  EXPECT_EQ(result.nextWriteIndex, writeIndex + input.size());
  EXPECT_FALSE(result.underflow);
  for (size_t i = 0; i < input.size(); ++i) {
    const size_t idx = (writeIndex + i) & mask;
    EXPECT_FLOAT_EQ(ring[idx], input[i]);
  }
}

TEST(ProcessCallbackInputTest, NullInputMarksUnderflowAndZeroFills) {
  std::vector<float> ring(8, 1.0f);
  const size_t mask = ring.size() - 1;
  const size_t writeIndex = 6;
  const size_t sampleCount = 5;

  auto result =
      avs::portaudio_detail::processCallbackInput(nullptr, sampleCount, writeIndex, mask, ring);

  EXPECT_EQ(result.nextWriteIndex, writeIndex + sampleCount);
  EXPECT_TRUE(result.underflow);
  for (size_t i = 0; i < sampleCount; ++i) {
    const size_t idx = (writeIndex + i) & mask;
    EXPECT_FLOAT_EQ(ring[idx], 0.0f);
  }
}

}  // namespace
