#include <avs/registry.hpp>
#include <avs/effects_render.hpp>
#include <avs/effects_trans.hpp>
#include <avs/effects_misc.hpp>

namespace avs {

std::unique_ptr<IEffect> Registry::create(std::string_view id) const {
  auto it = by_id_.find(std::string(id));
  if (it == by_id_.end()) return nullptr;
  const auto& descriptor = effects_.at(it->second);
  std::unique_ptr<IEffect> effect = descriptor.factory ? descriptor.factory() : nullptr;
  if (!effect) return nullptr;
  if (auto* list = dynamic_cast<EffectListEffect*>(effect.get())) {
    list->setFactory([this](std::string_view childId) { return this->create(childId); });
  }
  return effect;
}

// Helper macro to register effect with simple factory.
#define REG(ID, CLS, GRP)       do {         EffectDescriptor d;         d.id = ID;         d.label = #CLS;         d.group = GRP;         d.factory = [](){ return std::make_unique<CLS>(); };         r.register_effect(d);       } while(0)

void register_builtin_effects(Registry& r){
  // Render
  REG("oscilloscope", OscilloscopeEffect, EffectGroup::Render);
  REG("spectrum",     SpectrumAnalyzerEffect, EffectGroup::Render);
  REG("dots_lines",   DotsLinesEffect, EffectGroup::Render);
  REG("starfield",    StarfieldEffect, EffectGroup::Render);
  REG("text",         TextEffect, EffectGroup::Render);
  REG("picture",      PictureEffect, EffectGroup::Render);
  REG("superscope",   SuperscopeEffect, EffectGroup::Render);
  REG("triangles",    TrianglesEffect, EffectGroup::Render);
  REG("shapes",       ShapesEffect, EffectGroup::Render);
  REG("dot_grid",     DotGridEffect, EffectGroup::Render);

  // Trans
  REG("movement",       MovementEffect, EffectGroup::Trans);
  REG("dyn_movement",   DynamicMovementEffect, EffectGroup::Trans);
  REG("dyn_distance",   DynamicDistanceModifierEffect, EffectGroup::Trans);
  REG("dyn_shift",      DynamicShiftEffect, EffectGroup::Trans);
  REG("zoom_rotate",    ZoomRotateEffect, EffectGroup::Trans);
  REG("mirror",         MirrorEffect, EffectGroup::Trans);
  REG("conv3x3",        Convolution3x3Effect, EffectGroup::Trans);
  REG("blur_box",       BlurBoxEffect, EffectGroup::Trans);
  REG("color_map",      ColorMapEffect, EffectGroup::Trans);
  REG("invert",         InvertEffect, EffectGroup::Trans);
  REG("fadeout",        FadeoutEffect, EffectGroup::Trans);
  REG("bump",           BumpEffect, EffectGroup::Trans);
  REG("interferences",  InterferencesEffect, EffectGroup::Trans);
  REG("fast_brightness",FastBrightnessEffect, EffectGroup::Trans);
  REG("interleave",     InterleaveEffect, EffectGroup::Trans);
  REG("grain",          GrainEffect, EffectGroup::Trans);

  // Misc
  REG("effect_list",    EffectListEffect, EffectGroup::Misc);
  REG("globals",        GlobalVariablesEffect, EffectGroup::Misc);
  REG("save_buf",       SaveBufferEffect, EffectGroup::Misc);
  REG("restore_buf",    RestoreBufferEffect, EffectGroup::Misc);
  REG("onbeat_clear",   OnBeatClearEffect, EffectGroup::Misc);
  REG("clear_screen",   ClearScreenEffect, EffectGroup::Misc);
}

} // namespace avs
