#include "avs/effects_misc.hpp"
#include "avs/core.hpp"
#include "avs/params.hpp"
#include "avs/script/EELContext.hpp"
#include <algorithm>
#include <vector>

namespace avs {

namespace {

Param makeBoolParam(std::string name, bool value) {
  Param p;
  p.name = std::move(name);
  p.kind = ParamKind::Bool;
  p.value = value;
  return p;
}

Param makeFloatParam(std::string name, float value, float min, float max) {
  Param p;
  p.name = std::move(name);
  p.kind = ParamKind::Float;
  p.value = value;
  p.f_min = min;
  p.f_max = max;
  return p;
}

Param makeColorParam(std::string name, ColorRGBA8 value) {
  Param p;
  p.name = std::move(name);
  p.kind = ParamKind::Color;
  p.value = value;
  return p;
}

Param makeSelectParam(std::string name, std::string value, std::vector<OptionItem> options) {
  Param p;
  p.name = std::move(name);
  p.kind = ParamKind::Select;
  p.value = value;
  p.options = std::move(options);
  return p;
}

}  // namespace

// Effect List
void EffectListEffect::init(const InitContext&) {}

std::vector<Param> EffectListEffect::parameters() const {
  std::vector<Param> params;
  params.push_back(makeSelectParam("mode",
                                   "Blend",
                                   {OptionItem{"blend", "Blend"},
                                    OptionItem{"replace", "Replace"}}));
  params.push_back(makeFloatParam("weight", 0.5f, 0.0f, 1.0f));
  return params;
}

void EffectListEffect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  if (!ctx.fb.previous.data || !dst.data) return;
  const float weight = ctx.audio.beat ? 0.8f : 0.5f;
  for (int y = 0; y < dst.height; ++y) {
    uint8_t* dstRow = dst.data + y * dst.stride;
    const uint8_t* prevRow = ctx.fb.previous.data + y * ctx.fb.previous.stride;
    for (int x = 0; x < dst.width; ++x) {
      const size_t idx = static_cast<size_t>(x) * 4u;
      for (int c = 0; c < 3; ++c) {
        float a = dstRow[idx + c];
        float b = prevRow[idx + c];
        dstRow[idx + c] = static_cast<uint8_t>(std::clamp(a * (1.0f - weight) + b * weight, 0.0f, 255.0f));
      }
      dstRow[idx + 3] = 255;
    }
  }
}

// Global Variables
void GlobalVariablesEffect::init(const InitContext&) {}

std::vector<Param> GlobalVariablesEffect::parameters() const {
  std::vector<Param> params;
  params.push_back(makeBoolParam("expose_audio", true));
  params.push_back(makeBoolParam("expose_time", true));
  return params;
}

void GlobalVariablesEffect::process(const ProcessContext& ctx, FrameBufferView&) {
  if (!ctx.eel) return;
  ctx.eel->setVariable("time", ctx.time.t_seconds);
  ctx.eel->setVariable("frame", static_cast<double>(ctx.time.frame_index));
  ctx.eel->setVariable("bass", ctx.audio.bass);
  ctx.eel->setVariable("mid", ctx.audio.mid);
  ctx.eel->setVariable("treb", ctx.audio.treb);
  ctx.eel->setVariable("beat", ctx.audio.beat ? 1.0 : 0.0);
}

// Save / Restore buffer
std::vector<Param> SaveBufferEffect::parameters() const {
  std::vector<Param> params;
  params.push_back(makeSelectParam("slot",
                                   "previous",
                                   {OptionItem{"previous", "Previous"}}));
  return params;
}

void SaveBufferEffect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  if (!ctx.fb.previous.data || !dst.data) return;
  for (int y = 0; y < dst.height; ++y) {
    const uint8_t* srcRow = dst.data + y * dst.stride;
    uint8_t* dstRow = ctx.fb.previous.data + y * ctx.fb.previous.stride;
    std::copy(srcRow, srcRow + dst.width * 4, dstRow);
  }
}

std::vector<Param> RestoreBufferEffect::parameters() const {
  std::vector<Param> params;
  params.push_back(makeSelectParam("slot",
                                   "previous",
                                   {OptionItem{"previous", "Previous"}}));
  return params;
}

void RestoreBufferEffect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  if (!ctx.fb.previous.data || !dst.data) return;
  for (int y = 0; y < dst.height; ++y) {
    const uint8_t* srcRow = ctx.fb.previous.data + y * ctx.fb.previous.stride;
    uint8_t* dstRow = dst.data + y * dst.stride;
    std::copy(srcRow, srcRow + dst.width * 4, dstRow);
  }
}

// OnBeat Clear
std::vector<Param> OnBeatClearEffect::parameters() const {
  std::vector<Param> params;
  params.push_back(makeFloatParam("amount", 1.0f, 0.0f, 1.0f));
  return params;
}

void OnBeatClearEffect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  if (!ctx.audio.beat || !dst.data) return;
  const float amount = 1.0f;
  for (int y = 0; y < dst.height; ++y) {
    uint8_t* row = dst.data + y * dst.stride;
    for (int x = 0; x < dst.width; ++x) {
      const size_t idx = static_cast<size_t>(x) * 4u;
      row[idx + 0] = static_cast<uint8_t>(row[idx + 0] * (1.0f - amount));
      row[idx + 1] = static_cast<uint8_t>(row[idx + 1] * (1.0f - amount));
      row[idx + 2] = static_cast<uint8_t>(row[idx + 2] * (1.0f - amount));
      row[idx + 3] = 255;
    }
  }
}

// Clear Screen
std::vector<Param> ClearScreenEffect::parameters() const {
  std::vector<Param> params;
  params.push_back(makeColorParam("color", ColorRGBA8{0, 0, 0, 255}));
  return params;
}

void ClearScreenEffect::process(const ProcessContext&, FrameBufferView& dst) {
  const ColorRGBA8 color{0, 0, 0, 255};
  for (int y = 0; y < dst.height; ++y) {
    uint8_t* row = dst.data + y * dst.stride;
    for (int x = 0; x < dst.width; ++x) {
      const size_t idx = static_cast<size_t>(x) * 4u;
      row[idx + 0] = color.r;
      row[idx + 1] = color.g;
      row[idx + 2] = color.b;
      row[idx + 3] = color.a;
    }
  }
}

} // namespace avs
