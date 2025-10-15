#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "avs/core/EffectRegistry.hpp"
#include "avs/core/IEffect.hpp"
#include "avs/core/ParamBlock.hpp"
#include "avs/core/RenderContext.hpp"

namespace {

class DummyEffect : public avs::core::IEffect {
 public:
  bool render(avs::core::RenderContext&) override { return true; }
  void setParams(const avs::core::ParamBlock&) override {}
};

}  // namespace

TEST(EffectRegistryTest, RegistersFactoriesByKey) {
  avs::core::EffectRegistry registry;
  auto factory = []() { return std::make_unique<DummyEffect>(); };

  EXPECT_TRUE(registry.registerFactory("dummy", factory));
  auto effect = registry.make("dummy");
  ASSERT_NE(effect, nullptr);
  EXPECT_NE(nullptr, dynamic_cast<DummyEffect*>(effect.get()));
}

TEST(EffectRegistryTest, UnknownKeysReturnNullptr) {
  avs::core::EffectRegistry registry;
  EXPECT_EQ(nullptr, registry.make("missing"));
}

TEST(EffectRegistryTest, DuplicateRegistrationIsRejected) {
  avs::core::EffectRegistry registry;
  auto factory = []() { return std::make_unique<DummyEffect>(); };
  EXPECT_TRUE(registry.registerFactory("dummy", factory));
  EXPECT_FALSE(registry.registerFactory("dummy", factory));
}
