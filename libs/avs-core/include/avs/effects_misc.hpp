#pragma once
#include "effect.hpp"
#include "avs/runtime/framebuffers.h"

namespace avs {

// ---- Misc / Control Effects (declarations) ----

class EffectListEffect : public IEffect {
 public:
  EffectGroup group() const override { return EffectGroup::Misc; }
  std::string_view name() const override { return "Effect List"; }
  void init(const InitContext& ctx) override;
  void process(const ProcessContext& ctx, FrameBufferView& dst) override;
  std::vector<Param> parameters() const override;
  void set_parameter(std::string_view name, const ParamValue& value) override;

  using Factory = std::function<std::unique_ptr<IEffect>(std::string_view)>;
  void setFactory(Factory factory);
  // In implementation, hold child effects & their local output mode.

  struct ConfigNode {
    std::string id;
    std::vector<ConfigNode> children;
  };

 private:
  void rebuildChildren();

  Factory factory_{};
  std::vector<std::unique_ptr<IEffect>> children_;
  std::string config_;
  std::vector<ConfigNode> configTree_;
  bool initialized_{false};
  InitContext initContext_{};
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
  void set_parameter(std::string_view name, const ParamValue& value) override;

 private:
  runtime::BufferSlot slot_{runtime::BufferSlot::A};
};

class RestoreBufferEffect : public IEffect {
public:
  EffectGroup group() const override { return EffectGroup::Misc; }
  std::string_view name() const override { return "Restore Buffer"; }
  void process(const ProcessContext& ctx, FrameBufferView& dst) override;
  std::vector<Param> parameters() const override;
  void set_parameter(std::string_view name, const ParamValue& value) override;

 private:
  runtime::BufferSlot slot_{runtime::BufferSlot::A};
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
