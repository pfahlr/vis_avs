#include <gtest/gtest.h>

#include "effects/gating.h"

namespace {

using avs::effects::BeatGate;
using avs::effects::GateFlag;
using avs::effects::GateOptions;

}  // namespace

TEST(BeatGate, ActivatesOnBeatWithHold) {
  BeatGate gate;
  GateOptions options;
  options.enableOnBeat = true;
  options.holdFrames = 2;
  gate.configure(options);

  auto r0 = gate.step(true);
  EXPECT_TRUE(r0.render);
  EXPECT_EQ(r0.flag, GateFlag::Beat);

  auto r1 = gate.step(false);
  EXPECT_TRUE(r1.render);
  EXPECT_EQ(r1.flag, GateFlag::Hold);

  auto r2 = gate.step(false);
  EXPECT_FALSE(r2.render);
  EXPECT_EQ(r2.flag, GateFlag::Off);
}

TEST(BeatGate, StickyOnlyRequiresLatch) {
  BeatGate gate;
  GateOptions options;
  options.enableOnBeat = true;
  options.stickyToggle = true;
  options.onlySticky = true;
  gate.configure(options);

  auto r0 = gate.step(true);
  EXPECT_TRUE(r0.render);
  EXPECT_EQ(r0.flag, GateFlag::Sticky);

  auto r1 = gate.step(false);
  EXPECT_TRUE(r1.render);
  EXPECT_EQ(r1.flag, GateFlag::Sticky);

  auto r2 = gate.step(true);
  EXPECT_FALSE(r2.render);
  EXPECT_EQ(r2.flag, GateFlag::Beat);

  auto r3 = gate.step(false);
  EXPECT_FALSE(r3.render);
  EXPECT_EQ(r3.flag, GateFlag::Off);
}

