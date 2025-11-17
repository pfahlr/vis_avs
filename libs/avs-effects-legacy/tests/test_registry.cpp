#include <avs/effects/legacy/register_all.h>
#include <avs/effects/registry.h>
#include <gtest/gtest.h>

TEST(EffectsRegistry, NormalizesLegacyTokens) {
  EXPECT_EQ(avs::effects::Registry::normalize_legacy_token("Trans / Color Modifier"),
            "trans/color_modifier");
  EXPECT_EQ(avs::effects::Registry::normalize_legacy_token("  trans  /  COLOR modifier  "),
            "trans/color_modifier");
}

TEST(EffectsRegistry, MakesLegacyEffect) {
  avs::effects::Registry registry;
  avs::effects::legacy::register_all(registry);

  avs::effects::ParamList params;
  bool matchedLegacy = false;
  auto fx = registry.make("Trans / Color Modifier", params, {.compat = "strict"}, &matchedLegacy);
  ASSERT_NE(fx, nullptr);
  EXPECT_TRUE(matchedLegacy);
  EXPECT_STREQ(fx->id(), "trans/color_modifier");
}
