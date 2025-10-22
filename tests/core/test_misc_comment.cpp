#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <avs/core/ParamBlock.hpp>
#include <avs/core/RenderContext.hpp>
#include <avs/effects/micro_preset_parser.hpp>
#include "effects/misc/effect_comment.h"

namespace {

avs::core::RenderContext makeContext(std::vector<std::uint8_t>& pixels, int width, int height) {
  avs::core::RenderContext ctx{};
  ctx.width = width;
  ctx.height = height;
  ctx.framebuffer = {pixels.data(), pixels.size()};
  return ctx;
}

}  // namespace

TEST(MiscCommentEffectTest, StoresCommentFromParams) {
  avs::effects::misc::Comment effect;
  avs::core::ParamBlock params;
  params.setString("comment", "Hello World");

  effect.setParams(params);

  EXPECT_EQ(effect.text(), "Hello World");
}

TEST(MiscCommentEffectTest, RenderDoesNotChangeFramebuffer) {
  avs::effects::misc::Comment effect;
  avs::core::ParamBlock params;
  params.setString("comment", "noop");
  effect.setParams(params);

  std::vector<std::uint8_t> pixels{0, 1, 2, 3, 4, 5, 6, 7};
  auto before = pixels;
  auto ctx = makeContext(pixels, 2, 1);

  EXPECT_TRUE(effect.render(ctx));
  EXPECT_EQ(pixels, before);
}

TEST(MiscCommentEffectTest, MicroPresetParsesCommentTextVariants) {
  const std::string presetText =
      "MISC_COMMENT This is a note\n"
      "MISC_COMMENT comment=Hello world\n"
      "MISC_COMMENT comment=\"Value with spaces\" trailing tokens\n"
      "MISC_COMMENT comment='a=b'\n";

  const auto preset = avs::effects::parseMicroPreset(presetText);
  ASSERT_TRUE(preset.warnings.empty());
  ASSERT_EQ(preset.commands.size(), 4u);

  EXPECT_EQ(preset.commands[0].effectKey, "misc_comment");
  EXPECT_EQ(preset.commands[0].params.getString("comment", ""), "This is a note");

  EXPECT_EQ(preset.commands[1].effectKey, "misc_comment");
  EXPECT_EQ(preset.commands[1].params.getString("comment", ""), "Hello world");

  EXPECT_EQ(preset.commands[2].effectKey, "misc_comment");
  EXPECT_EQ(preset.commands[2].params.getString("comment", ""),
            "Value with spaces trailing tokens");

  EXPECT_EQ(preset.commands[3].effectKey, "misc_comment");
  EXPECT_EQ(preset.commands[3].params.getString("comment", ""), "a=b");
}
