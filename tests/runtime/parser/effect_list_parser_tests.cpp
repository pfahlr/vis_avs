#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "avs/effect.hpp"
#include "avs/effects_misc.hpp"

namespace {

class CountingEffect : public avs::IEffect {
 public:
  explicit CountingEffect(int& counter) : counter_(&counter) { ++(*counter_); }

  avs::EffectGroup group() const override { return avs::EffectGroup::Misc; }
  std::string_view name() const override { return "counting"; }
  void process(const avs::ProcessContext&, avs::FrameBufferView&) override {}

 private:
  int* counter_;
};

}  // namespace

TEST(EffectListConfigParser, AcceptsEmptyArray) {
  avs::EffectListEffect effect;
  int constructed = 0;
  std::vector<std::string> ids;
  effect.setFactory([&](std::string_view id) {
    ids.emplace_back(id);
    return std::make_unique<CountingEffect>(constructed);
  });

  effect.set_parameter("config", std::string("[]"));

  EXPECT_TRUE(ids.empty());
  EXPECT_EQ(constructed, 0);
}

TEST(EffectListConfigParser, ParsesSingleEffect) {
  avs::EffectListEffect effect;
  int constructed = 0;
  std::vector<std::string> ids;
  effect.setFactory([&](std::string_view id) {
    ids.emplace_back(id);
    return std::make_unique<CountingEffect>(constructed);
  });

  effect.set_parameter("config", std::string("[{\"effect\":\"foo\"}]"));

  ASSERT_EQ(ids.size(), 1u);
  EXPECT_EQ(ids.front(), "foo");
  EXPECT_EQ(constructed, 1);
}

TEST(EffectListConfigParser, ParsesMultipleEffects) {
  avs::EffectListEffect effect;
  int constructed = 0;
  std::vector<std::string> ids;
  effect.setFactory([&](std::string_view id) {
    ids.emplace_back(id);
    return std::make_unique<CountingEffect>(constructed);
  });

  effect.set_parameter(
      "config", std::string("[{\"effect\":\"foo\"},{\"effect\":\"bar\"}]"));

  ASSERT_EQ(ids.size(), 2u);
  EXPECT_EQ(ids[0], "foo");
  EXPECT_EQ(ids[1], "bar");
  EXPECT_EQ(constructed, 2);
}

TEST(EffectListConfigParser, RejectsInvalidArray) {
  avs::EffectListEffect effect;
  int constructed = 0;
  std::vector<std::string> ids;
  effect.setFactory([&](std::string_view id) {
    ids.emplace_back(id);
    return std::make_unique<CountingEffect>(constructed);
  });

  effect.set_parameter("config", std::string("[invalid]"));

  EXPECT_TRUE(ids.empty());
  EXPECT_EQ(constructed, 0);
}
