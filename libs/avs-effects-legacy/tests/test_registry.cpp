#include <avs/effects/registry.h>
#include <gtest/gtest.h>

TEST(EffectsRegistry, NormalizesLegacyTokens) {
  EXPECT_EQ(avs::effects::Registry::normalize_legacy_token("Trans / Color Modifier"),
            "trans/color_modifier");
  EXPECT_EQ(avs::effects::Registry::normalize_legacy_token("  trans  /  COLOR modifier  "),
            "trans/color_modifier");
  EXPECT_EQ(avs::effects::Registry::normalize_legacy_token("Render / Superscope"),
            "render/superscope");
  EXPECT_EQ(avs::effects::Registry::normalize_legacy_token("Misc / Set render size"),
            "misc/set_render_size");
}
