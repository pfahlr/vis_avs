#include "effects/misc/effect_custom_bpm.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "avs/core/ParamBlock.hpp"
#include "avs/core/RenderContext.hpp"
#include "avs/runtime/GlobalState.hpp"
#include "audio/analyzer.h"

using avs::effects::misc::CustomBpmEffect;

namespace {

avs::core::RenderContext makeContext() {
  avs::core::RenderContext ctx{};
  ctx.deltaSeconds = 0.25;
  ctx.audioBeat = false;
  static avs::audio::Analysis analysis{};
  analysis = avs::audio::Analysis{};
  ctx.audioAnalysis = &analysis;
  return ctx;
}

avs::audio::Analysis& analysisFor(avs::core::RenderContext& ctx) {
  return *const_cast<avs::audio::Analysis*>(ctx.audioAnalysis);
}

}  // namespace

TEST(CustomBpmEffectTest, ArbitraryTempoGeneratesBeats) {
  CustomBpmEffect effect;
  avs::core::ParamBlock params;
  params.setBool("arbitrary", true);
  params.setFloat("bpm", 60.0f);
  effect.setParams(params);

  avs::core::RenderContext ctx = makeContext();
  std::vector<bool> pulses;
  for (int i = 0; i < 8; ++i) {
    ctx.frameIndex = static_cast<std::uint64_t>(i);
    ctx.audioBeat = false;
    analysisFor(ctx).beat = false;
    effect.render(ctx);
    pulses.push_back(ctx.audioBeat);
  }

  ASSERT_EQ(pulses.size(), 8u);
  EXPECT_FALSE(pulses[0]);
  EXPECT_FALSE(pulses[1]);
  EXPECT_FALSE(pulses[2]);
  EXPECT_TRUE(pulses[3]);
  EXPECT_FALSE(pulses[4]);
  EXPECT_FALSE(pulses[5]);
  EXPECT_FALSE(pulses[6]);
  EXPECT_TRUE(pulses[7]);
}

TEST(CustomBpmEffectTest, SkipModeDropsInterveningBeats) {
  CustomBpmEffect effect;
  avs::core::ParamBlock params;
  params.setBool("arbitrary", false);
  params.setBool("skip", true);
  params.setInt("skip_val", 1);
  effect.setParams(params);

  avs::core::RenderContext ctx = makeContext();
  std::vector<bool> inputs = {true, false, true, false, true};
  std::vector<bool> outputs;
  for (std::size_t i = 0; i < inputs.size(); ++i) {
    ctx.frameIndex = static_cast<std::uint64_t>(i);
    ctx.audioBeat = inputs[i];
    analysisFor(ctx).beat = inputs[i];
    effect.render(ctx);
    outputs.push_back(ctx.audioBeat);
  }

  ASSERT_EQ(outputs.size(), inputs.size());
  EXPECT_FALSE(outputs[0]);
  EXPECT_FALSE(outputs[1]);
  EXPECT_TRUE(outputs[2]);
  EXPECT_FALSE(outputs[3]);
  EXPECT_FALSE(outputs[4]);
}

TEST(CustomBpmEffectTest, InvertModeFlipsBeats) {
  CustomBpmEffect effect;
  avs::core::ParamBlock params;
  params.setBool("arbitrary", false);
  params.setBool("invert", true);
  effect.setParams(params);

  avs::core::RenderContext ctx = makeContext();
  std::vector<bool> inputs = {true, false, false, true};
  std::vector<bool> outputs;
  for (std::size_t i = 0; i < inputs.size(); ++i) {
    ctx.frameIndex = static_cast<std::uint64_t>(i);
    ctx.audioBeat = inputs[i];
    analysisFor(ctx).beat = inputs[i];
    effect.render(ctx);
    outputs.push_back(ctx.audioBeat);
  }

  ASSERT_EQ(outputs.size(), inputs.size());
  EXPECT_FALSE(outputs[0]);
  EXPECT_TRUE(outputs[1]);
  EXPECT_TRUE(outputs[2]);
  EXPECT_FALSE(outputs[3]);
}

TEST(CustomBpmEffectTest, SkipFirstClearsInitialBeats) {
  CustomBpmEffect effect;
  avs::core::ParamBlock params;
  params.setBool("arbitrary", false);
  params.setInt("skip_first", 2);
  effect.setParams(params);

  avs::core::RenderContext ctx = makeContext();
  std::vector<bool> inputs = {true, true, true, false};
  std::vector<bool> outputs;
  for (std::size_t i = 0; i < inputs.size(); ++i) {
    ctx.frameIndex = static_cast<std::uint64_t>(i);
    ctx.audioBeat = inputs[i];
    analysisFor(ctx).beat = inputs[i];
    effect.render(ctx);
    outputs.push_back(ctx.audioBeat);
  }

  ASSERT_EQ(outputs.size(), inputs.size());
  EXPECT_FALSE(outputs[0]);
  EXPECT_FALSE(outputs[1]);
  EXPECT_TRUE(outputs[2]);
  EXPECT_FALSE(outputs[3]);
}

TEST(CustomBpmEffectTest, GateRegistersReflectHoldState) {
  CustomBpmEffect effect;
  avs::core::ParamBlock params;
  params.setBool("arbitrary", true);
  params.setFloat("bpm", 60.0f);
  params.setInt("gate_hold", 2);
  params.setInt("gate_register", 1);
  params.setInt("gate_flag_register", 2);
  effect.setParams(params);

  avs::core::RenderContext ctx = makeContext();
  avs::runtime::GlobalState globals;
  ctx.globals = &globals;

  std::vector<int> flagHistory;
  std::vector<int> renderHistory;
  for (int i = 0; i < 6; ++i) {
    ctx.frameIndex = static_cast<std::uint64_t>(i);
    ctx.audioBeat = false;
    analysisFor(ctx).beat = false;
    effect.render(ctx);
    renderHistory.push_back(static_cast<int>(ctx.globals->registers[0]));
    flagHistory.push_back(static_cast<int>(ctx.globals->registers[1]));
  }

  ASSERT_EQ(renderHistory.size(), 6u);
  ASSERT_EQ(flagHistory.size(), 6u);
  // Beat triggers on the fourth frame (0.75 -> 1.0s boundary).
  EXPECT_EQ(renderHistory[0], 0);
  EXPECT_EQ(flagHistory[0], 0);
  EXPECT_EQ(renderHistory[1], 0);
  EXPECT_EQ(flagHistory[1], 0);
  EXPECT_EQ(renderHistory[2], 0);
  EXPECT_EQ(flagHistory[2], 0);
  EXPECT_EQ(renderHistory[3], 1);
  EXPECT_EQ(flagHistory[3], 1);
  EXPECT_EQ(renderHistory[4], 1);
  EXPECT_EQ(flagHistory[4], 2);
  EXPECT_EQ(renderHistory[5], 0);
  EXPECT_EQ(flagHistory[5], 0);
}
