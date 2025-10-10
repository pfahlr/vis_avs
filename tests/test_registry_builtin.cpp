#include <gtest/gtest.h>

#include "avs/registry.hpp"

namespace avs::test {

TEST(RegistryTest, RegistersBuiltinEffects) {
  Registry registry;
  register_builtin_effects(registry);
  ASSERT_FALSE(registry.effects().empty());

  auto oscilloscope = registry.create("oscilloscope");
  auto movement = registry.create("movement");
  auto clear = registry.create("clear_screen");

  EXPECT_NE(nullptr, oscilloscope);
  EXPECT_NE(nullptr, movement);
  EXPECT_NE(nullptr, clear);
  EXPECT_EQ(EffectGroup::Render, oscilloscope->group());
  EXPECT_EQ(EffectGroup::Trans, movement->group());
  EXPECT_EQ(EffectGroup::Misc, clear->group());
}

}  // namespace avs::test

