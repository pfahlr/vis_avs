#pragma once
#include "effect.hpp"

namespace avs {

// ---- Misc / Control Effects (declarations) ----

class EffectListEffect : public IEffect {
public:
  EffectGroup group() const override { return EffectGroup::Misc; }
  std::string_view name() const override { return "Effect List"; }
  void init(const InitContext& ctx) override;
  void process(const ProcessContext& ctx, FrameBufferView& dst) override;
  std::vector<Param> parameters() const override;
  // In implementation, hold child effects & their local output mode.
};

class GlobalVariablesEffect : public IEffect {
public:
  EffectGroup group() const override { return EffectGroup::Misc; }
  std::string_view name() const override { return "Global Variables"; }
  void init(const InitContext& ctx) override;
  void process(const ProcessContext& ctx, FrameBufferView& dst) override;
  std::vector<Param> parameters() const override;
};

class SaveBufferEffect : public IEffect {
public:
  EffectGroup group() const override { return EffectGroup::Misc; }
  std::string_view name() const override { return "Save Buffer"; }
  void process(const ProcessContext& ctx, FrameBufferView& dst) override;
  std::vector<Param> parameters() const override;
};

class RestoreBufferEffect : public IEffect {
public:
  EffectGroup group() const override { return EffectGroup::Misc; }
  std::string_view name() const override { return "Restore Buffer"; }
  void process(const ProcessContext& ctx, FrameBufferView& dst) override;
  std::vector<Param> parameters() const override;
};

class OnBeatClearEffect : public IEffect {
public:
  EffectGroup group() const override { return EffectGroup::Misc; }
  std::string_view name() const override { return "OnBeat Clear"; }
  void process(const ProcessContext& ctx, FrameBufferView& dst) override;
  std::vector<Param> parameters() const override;
};

class ClearScreenEffect : public IEffect {
public:
  EffectGroup group() const override { return EffectGroup::Misc; }
  std::string_view name() const override { return "Clear Screen"; }
  void process(const ProcessContext& ctx, FrameBufferView& dst) override;
  std::vector<Param> parameters() const override;
};

} // namespace avs
