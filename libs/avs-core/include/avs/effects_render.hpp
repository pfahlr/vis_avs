#pragma once
#include "effect.hpp"
#include <vector>

namespace avs {

// ---- Render Effects (declarations) ----

class OscilloscopeEffect : public IEffect {
public:
  EffectGroup group() const override { return EffectGroup::Render; }
  std::string_view name() const override { return "Oscilloscope"; }
  void process(const ProcessContext& ctx, FrameBufferView& dst) override;
  std::vector<Param> parameters() const override;
};

class SpectrumAnalyzerEffect : public IEffect {
public:
  EffectGroup group() const override { return EffectGroup::Render; }
  std::string_view name() const override { return "Spectrum Analyzer"; }
  void process(const ProcessContext& ctx, FrameBufferView& dst) override;
  std::vector<Param> parameters() const override;
};

class DotsLinesEffect : public IEffect {
public:
  EffectGroup group() const override { return EffectGroup::Render; }
  std::string_view name() const override { return "Dots/Lines"; }
  void process(const ProcessContext& ctx, FrameBufferView& dst) override;
  std::vector<Param> parameters() const override;
};

class StarfieldEffect : public IEffect {
public:
  EffectGroup group() const override { return EffectGroup::Render; }
  std::string_view name() const override { return "Starfield"; }
  void process(const ProcessContext& ctx, FrameBufferView& dst) override;
  std::vector<Param> parameters() const override;
};

class TextEffect : public IEffect {
public:
  EffectGroup group() const override { return EffectGroup::Render; }
  std::string_view name() const override { return "Text"; }
  void process(const ProcessContext& ctx, FrameBufferView& dst) override;
  std::vector<Param> parameters() const override;
};

class PictureEffect : public IEffect {
public:
  EffectGroup group() const override { return EffectGroup::Render; }
  std::string_view name() const override { return "Picture"; }
  void process(const ProcessContext& ctx, FrameBufferView& dst) override;
  std::vector<Param> parameters() const override;
  void set_parameter(std::string_view name, const ParamValue& value) override;

private:
  bool loadImage();

  std::string path_;
  std::vector<std::uint8_t> image_;
  int imageWidth_{0};
  int imageHeight_{0};
  bool dirty_{true};
};

class SuperscopeEffect : public IEffect {
public:
  EffectGroup group() const override { return EffectGroup::Render; }
  std::string_view name() const override { return "Superscope"; }
  void init(const InitContext& ctx) override;
  void process(const ProcessContext& ctx, FrameBufferView& dst) override;
  std::vector<Param> parameters() const override;
};

class TrianglesEffect : public IEffect {
public:
  EffectGroup group() const override { return EffectGroup::Render; }
  std::string_view name() const override { return "Triangles"; }
  void process(const ProcessContext& ctx, FrameBufferView& dst) override;
  std::vector<Param> parameters() const override;
};

class ShapesEffect : public IEffect {
public:
  EffectGroup group() const override { return EffectGroup::Render; }
  std::string_view name() const override { return "Shapes"; }
  void process(const ProcessContext& ctx, FrameBufferView& dst) override;
  std::vector<Param> parameters() const override;
};

class DotGridEffect : public IEffect {
public:
  EffectGroup group() const override { return EffectGroup::Render; }
  std::string_view name() const override { return "Dot Grid"; }
  void process(const ProcessContext& ctx, FrameBufferView& dst) override;
  std::vector<Param> parameters() const override;
};

} // namespace avs
