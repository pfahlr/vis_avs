#include <avs/preset/binder.h>
#include <gtest/gtest.h>

using namespace avs;

// Note: register_all() has been removed since preset loading now uses
// AVS_LEGACY_REGISTER_EFFECT macro system for binary presets.
// These tests verify the binder creates fallback effects for unknown tokens.

TEST(PresetBinder, UnknownEffectCreatesFallback) {
  // Empty registry - all effects will be unknown
  avs::effects::Registry registry;

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

TEST(PresetBinder, MultipleUnknownEffectsCreateFallbacks) {
  avs::effects::Registry registry;

  avs::preset::IRNode node1;
  node1.token = "Trans / Color Modifier";
  avs::preset::IRNode node2;
  node2.token = "Render / Superscope";

  avs::preset::IRPreset preset;
  preset.root_nodes.push_back(node1);
  preset.root_nodes.push_back(node2);

  auto graph = avs::preset::bind_preset(preset, {}, registry);
  ASSERT_NE(graph, nullptr);
  EXPECT_EQ(graph->size(), 2u);
}
