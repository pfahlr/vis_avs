#pragma once
#include "effect.hpp"
#include <vector>

namespace avs {

// ---- Transform Effects (declarations) ----

class MovementEffect : public IEffect {
public:
  EffectGroup group() const override { return EffectGroup::Trans; }
  std::string_view name() const override { return "Movement"; }
  void init(const InitContext& ctx) override;
  void process(const ProcessContext& ctx, FrameBufferView& dst) override;
  std::vector<Param> parameters() const override;
};

class DynamicMovementEffect : public IEffect {
public:
  EffectGroup group() const override { return EffectGroup::Trans; }
  std::string_view name() const override { return "Dynamic Movement"; }
  void init(const InitContext& ctx) override;
  void process(const ProcessContext& ctx, FrameBufferView& dst) override;
  std::vector<Param> parameters() const override;
};

class DynamicDistanceModifierEffect : public IEffect {
public:
  EffectGroup group() const override { return EffectGroup::Trans; }
  std::string_view name() const override { return "Dynamic Distance Modifier"; }
  void init(const InitContext& ctx) override;
  void process(const ProcessContext& ctx, FrameBufferView& dst) override;
  std::vector<Param> parameters() const override;
};

class DynamicShiftEffect : public IEffect {
public:
  EffectGroup group() const override { return EffectGroup::Trans; }
  std::string_view name() const override { return "Dynamic Shift"; }
  void init(const InitContext& ctx) override;
  void process(const ProcessContext& ctx, FrameBufferView& dst) override;
  std::vector<Param> parameters() const override;
};

class ZoomRotateEffect : public IEffect {
public:
  EffectGroup group() const override { return EffectGroup::Trans; }
  std::string_view name() const override { return "Zoom/Rotate"; }
  void process(const ProcessContext& ctx, FrameBufferView& dst) override;
  std::vector<Param> parameters() const override;
};

class MirrorEffect : public IEffect {
public:
  EffectGroup group() const override { return EffectGroup::Trans; }
  std::string_view name() const override { return "Mirror"; }
  void process(const ProcessContext& ctx, FrameBufferView& dst) override;
  std::vector<Param> parameters() const override;
  void set_parameter(std::string_view name, const ParamValue& value) override;

private:
  enum class Mode { Horizontal = 0, Vertical = 1, Quad = 2 };
  Mode mode_{Mode::Horizontal};
};

class Convolution3x3Effect : public IEffect {
public:
  EffectGroup group() const override { return EffectGroup::Trans; }
  std::string_view name() const override { return "Convolution 3x3"; }
  void process(const ProcessContext& ctx, FrameBufferView& dst) override;
  std::vector<Param> parameters() const override;
};

class BlurBoxEffect : public IEffect {
public:
  EffectGroup group() const override { return EffectGroup::Trans; }
  std::string_view name() const override { return "Blur (Box)"; }
  void process(const ProcessContext& ctx, FrameBufferView& dst) override;
  std::vector<Param> parameters() const override;
};

class ColorMapEffect : public IEffect {
public:
  EffectGroup group() const override { return EffectGroup::Trans; }
  std::string_view name() const override { return "Color Map"; }
  void process(const ProcessContext& ctx, FrameBufferView& dst) override;
  std::vector<Param> parameters() const override;
};

class InvertEffect : public IEffect {
public:
  EffectGroup group() const override { return EffectGroup::Trans; }
  std::string_view name() const override { return "Invert"; }
  void process(const ProcessContext& ctx, FrameBufferView& dst) override;
};

class FadeoutEffect : public IEffect {
public:
  EffectGroup group() const override { return EffectGroup::Trans; }
  std::string_view name() const override { return "Fadeout"; }
  void process(const ProcessContext& ctx, FrameBufferView& dst) override;
  std::vector<Param> parameters() const override;
};

class BumpEffect : public IEffect {
public:
  EffectGroup group() const override { return EffectGroup::Trans; }
  std::string_view name() const override { return "Bump"; }
  void process(const ProcessContext& ctx, FrameBufferView& dst) override;
  std::vector<Param> parameters() const override;
};

class InterferencesEffect : public IEffect {
public:
  EffectGroup group() const override { return EffectGroup::Trans; }
  std::string_view name() const override { return "Interferences"; }
  void process(const ProcessContext& ctx, FrameBufferView& dst) override;
  std::vector<Param> parameters() const override;
};

class FastBrightnessEffect : public IEffect {
public:
  EffectGroup group() const override { return EffectGroup::Trans; }
  std::string_view name() const override { return "Fast Brightness"; }
  void process(const ProcessContext& ctx, FrameBufferView& dst) override;
  std::vector<Param> parameters() const override;
};

class GrainEffect : public IEffect {
public:
  EffectGroup group() const override { return EffectGroup::Trans; }
  std::string_view name() const override { return "Grain"; }
  void process(const ProcessContext& ctx, FrameBufferView& dst) override;
  std::vector<Param> parameters() const override;
};

class InterleaveEffect : public IEffect {
public:
  EffectGroup group() const override { return EffectGroup::Trans; }
  std::string_view name() const override { return "Interleave"; }
  void process(const ProcessContext& ctx, FrameBufferView& dst) override;
  std::vector<Param> parameters() const override;
  void set_parameter(std::string_view name, const ParamValue& value) override;

private:
  void ensureHistory(int width, int height, int stride);

  int frameCount_{2};
  int offset_{0};
  int width_{0};
  int height_{0};
  int stride_{0};
  std::vector<std::vector<std::uint8_t>> frames_;
  std::vector<bool> ready_;
};

} // namespace avs

