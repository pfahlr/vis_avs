#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <avs/effect.hpp>
#include <avs/effects_misc.hpp>

namespace {

class CountingEffect : public avs::IEffect {
 public:
  explicit CountingEffect(int* counter) : counter_(counter) {}

  avs::EffectGroup group() const override { return avs::EffectGroup::Misc; }
  std::string_view name() const override { return "Counting"; }
  void process(const avs::ProcessContext&, avs::FrameBufferView&) override {
    if (counter_ != nullptr) {
      ++(*counter_);
    }
  }
  std::vector<avs::Param> parameters() const override { return {}; }
  void set_parameter(std::string_view, const avs::ParamValue&) override {}

 private:
  int* counter_{nullptr};
};

struct EffectHarness {
  avs::EffectListEffect effect;
  int processCount{0};
  std::vector<std::string> created;

  EffectHarness() {
    effect.setFactory([this](std::string_view id) {
      created.emplace_back(id);
      return std::make_unique<CountingEffect>(&processCount);
    });
  }
};

avs::ProcessContext makeProcessContext() {
  static avs::TimingInfo timing{};
  static avs::AudioFeatures audio{};
  static avs::FrameBuffers buffers{};
  return avs::ProcessContext{timing, audio, buffers, nullptr, nullptr};
}

}  // namespace

TEST(EffectListConfigParser, ParsesEmptyArray) {
  EffectHarness harness;
  avs::FrameBufferView dst{};
  auto ctx = makeProcessContext();

  harness.effect.set_parameter("config", std::string("[]"));
  harness.effect.process(ctx, dst);

  EXPECT_TRUE(harness.created.empty());
  EXPECT_EQ(harness.processCount, 0);
}

TEST(EffectListConfigParser, ParsesSingleEffectArray) {
  EffectHarness harness;
  avs::FrameBufferView dst{};
  auto ctx = makeProcessContext();

  harness.effect.set_parameter("config", std::string("[{\"effect\":\"foo\"}]"));
  harness.effect.process(ctx, dst);

  ASSERT_EQ(harness.created.size(), 1u);
  EXPECT_EQ(harness.created.front(), "foo");
  EXPECT_EQ(harness.processCount, 1);
}

TEST(EffectListConfigParser, ParsesMultipleEffectArray) {
  EffectHarness harness;
  avs::FrameBufferView dst{};
  auto ctx = makeProcessContext();

  harness.effect.set_parameter(
      "config", std::string("[{\"effect\":\"foo\"},{\"effect\":\"bar\"}]"));
  harness.effect.process(ctx, dst);

  ASSERT_EQ(harness.created.size(), 2u);
  EXPECT_EQ(harness.created[0], "foo");
  EXPECT_EQ(harness.created[1], "bar");
  EXPECT_EQ(harness.processCount, 2);
}

TEST(EffectListConfigParser, InvalidArrayClearsChildren) {
  EffectHarness harness;
  avs::FrameBufferView dst{};
  auto ctx = makeProcessContext();

  harness.effect.set_parameter("config", std::string("[{\"effect\":\"foo\"}]"));
  harness.effect.process(ctx, dst);
  ASSERT_EQ(harness.processCount, 1);

  harness.created.clear();
  harness.processCount = 0;
  harness.effect.set_parameter("config", std::string("[invalid]"));
  harness.effect.process(ctx, dst);

  EXPECT_TRUE(harness.created.empty());
  EXPECT_EQ(harness.processCount, 0);
}
