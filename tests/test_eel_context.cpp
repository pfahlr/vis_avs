#include <gtest/gtest.h>

#include "avs/script/EELContext.hpp"

#if AVS_ENABLE_EEL2

namespace avs::test {

TEST(EELContextTest, CompilesAndRunsSimpleScript) {
  EELContext ctx;
  ASSERT_TRUE(ctx.isEnabled());

  ctx.setVariable("time", 0.0);
  ASSERT_TRUE(ctx.compile("tick", "time = time + 1;"));
  EXPECT_TRUE(ctx.execute("tick"));
  EXPECT_DOUBLE_EQ(1.0, ctx.getVariable("time"));

  AudioFeatures audio;
  audio.oscL = {0.0f, 0.5f, 1.0f};
  audio.oscR = {0.25f, -0.25f, 0.75f};
  audio.spectrum.left = {0.1f, 0.2f, 0.3f};
  audio.spectrum.right = {0.4f, 0.5f, 0.6f};

  ctx.updateAudio(audio, 0.0);
  ctx.setVariable("result", 0.0);
  ASSERT_TRUE(ctx.compile("sample", "result = getosc(0, 0, 0);"));
  EXPECT_TRUE(ctx.execute("sample"));
  double value = ctx.getVariable("result");
  EXPECT_GE(value, -1.0);
  EXPECT_LE(value, 1.0);
}

}  // namespace avs::test

#endif  // AVS_ENABLE_EEL2

