#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include <avs/audio/analyzer.h>

namespace {

using avs::audio::Analysis;
using avs::audio::Analyzer;

TEST(AudioAnalyzerTest, ProcessesNewSamplesWhenDampingDisabled) {
  Analyzer analyzer{44100, 1};
  analyzer.setDampingEnabled(false);

  std::vector<float> frame(Analysis::kFftSize * 1u, 0.0f);
  analyzer.process(frame.data(), Analysis::kFftSize);

  std::fill(frame.begin(), frame.end(), 1.0f);
  const Analysis& analysis = analyzer.process(frame.data(), Analysis::kFftSize);

  const bool hasNonZeroWaveform = std::any_of(analysis.waveform.begin(), analysis.waveform.end(),
                                              [](float value) { return std::abs(value) > 1e-6f; });

  EXPECT_TRUE(hasNonZeroWaveform);
  EXPECT_GT(analysis.spectrum.front(), 0.0f);
}

}  // namespace
