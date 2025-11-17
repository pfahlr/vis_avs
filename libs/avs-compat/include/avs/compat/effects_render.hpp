#pragma once
#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "effect.hpp"
#include <vector>

namespace avs::effects::geometry {
class SuperscopeRuntime;
struct SuperscopeRuntimeDeleter {
  void operator()(SuperscopeRuntime* ptr) const noexcept;
};
}  // namespace avs::effects::geometry

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
  void set_parameter(std::string_view name, const ParamValue& value) override;

 private:
  struct Settings {
    std::string text;
    int x{0};
    int y{0};
    int size{16};
    int glyphWidth{0};
    int spacing{1};
    ColorRGBA8 color{255, 255, 255, 255};
    ColorRGBA8 outline{0, 0, 0, 255};
    int outlineSize{0};
    bool shadow{false};
    ColorRGBA8 shadowColor{0, 0, 0, 128};
    int shadowOffsetX{2};
    int shadowOffsetY{2};
    int shadowBlur{2};
    bool antialias{false};
    std::string halign{"left"};
    std::string valign{"top"};
  } settings_;
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
  ~SuperscopeEffect() override;
  EffectGroup group() const override { return EffectGroup::Render; }
  std::string_view name() const override { return "Superscope"; }
  void init(const InitContext& ctx) override;
  void process(const ProcessContext& ctx, FrameBufferView& dst) override;
  std::vector<Param> parameters() const override;
  void set_parameter(std::string_view name, const ParamValue& value) override;

 private:
  std::unique_ptr<effects::geometry::SuperscopeRuntime, effects::geometry::SuperscopeRuntimeDeleter>
      runtime_;
  std::string initScript_;
  std::string frameScript_;
  std::string beatScript_;
  std::string pointScript_;
  std::optional<int> overridePoints_;
  std::optional<float> overrideThickness_;
  std::optional<bool> overrideLineMode_;
  bool initialized_{false};
};

class TrianglesEffect : public IEffect {
 public:
  EffectGroup group() const override { return EffectGroup::Render; }
  std::string_view name() const override { return "Triangles"; }
  void process(const ProcessContext& ctx, FrameBufferView& dst) override;
  std::vector<Param> parameters() const override;
  void set_parameter(std::string_view name, const ParamValue& value) override;

 private:
  struct Triangle {
    Vec2i a;
    Vec2i b;
    Vec2i c;
  };
  std::vector<Triangle> triangles_;
  bool filled_{true};
  ColorRGBA8 fillColor_{255, 255, 255, 255};
  ColorRGBA8 outlineColor_{0, 0, 0, 255};
  int outlineWidth_{0};
  std::array<Vec2i, 3> pendingVertices_{{Vec2i{0, 0}, Vec2i{0, 0}, Vec2i{0, 0}}};
  std::array<bool, 3> pendingMask_{{false, false, false}};
};

class ShapesEffect : public IEffect {
 public:
  EffectGroup group() const override { return EffectGroup::Render; }
  std::string_view name() const override { return "Shapes"; }
  void process(const ProcessContext& ctx, FrameBufferView& dst) override;
  std::vector<Param> parameters() const override;
  void set_parameter(std::string_view name, const ParamValue& value) override;

 private:
  enum class ShapeType { Circle, Rect, Star, Line };
  struct ShapeSettings {
    ShapeType type{ShapeType::Circle};
    int x{0};
    int y{0};
    int radius{50};
    int width{100};
    int height{100};
    int innerRadius{25};
    int points{5};
    float rotationDeg{0.0f};
    bool filled{true};
    ColorRGBA8 fillColor{255, 255, 255, 255};
    ColorRGBA8 outlineColor{0, 0, 0, 255};
    int outlineWidth{0};
    Vec2i lineEnd{0, 0};
    bool lineEndSet{false};
  } settings_;
};

class DotGridEffect : public IEffect {
 public:
  EffectGroup group() const override { return EffectGroup::Render; }
  std::string_view name() const override { return "Dot Grid"; }
  void process(const ProcessContext& ctx, FrameBufferView& dst) override;
  std::vector<Param> parameters() const override;
  void set_parameter(std::string_view name, const ParamValue& value) override;

 private:
  struct GridSettings {
    int cols{8};
    int rows{8};
    int spacingX{32};
    int spacingY{32};
    int offsetX{16};
    int offsetY{16};
    int radius{4};
    bool alternate{false};
    ColorRGBA8 colorA{255, 255, 255, 255};
    ColorRGBA8 colorB{128, 128, 128, 255};
  } settings_;
};

}  // namespace avs

