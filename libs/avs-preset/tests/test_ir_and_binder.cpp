#include <avs/effects/legacy/register_all.h>
#include <avs/preset/binder.h>
#include <gtest/gtest.h>

using namespace avs;

TEST(PresetBinder, BindsKnownEffect) {
  avs::effects::Registry registry;
  avs::effects::legacy::register_all(registry);

  avs::preset::IRNode node;
  node.token = "Trans / Color Modifier";
  avs::preset::IRPreset preset;
  preset.root_nodes.push_back(node);

  avs::preset::BindOptions options;
  auto graph = avs::preset::bind_preset(preset, options, registry);
  ASSERT_NE(graph, nullptr);
  EXPECT_GE(graph->size(), 1u);
}

TEST(PresetBinder, UnknownEffectCreatesFallback) {
  avs::effects::Registry registry;
  avs::effects::legacy::register_all(registry);

  avs::preset::IRNode node;
  node.token = "Render / Nonexistent";
  avs::preset::IRPreset preset;
  preset.root_nodes.push_back(node);

  auto graph = avs::preset::bind_preset(preset, {}, registry);
  ASSERT_NE(graph, nullptr);
  ASSERT_EQ(graph->size(), 1u);
  ASSERT_NE(graph->nodes()[0], nullptr);
  EXPECT_STREQ(graph->nodes()[0]->id(), "Render / Nonexistent");
}
