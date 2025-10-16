#include "avs/effects_trans.hpp"

#include <algorithm>

#include "avs/core.hpp"

namespace avs {

// Simple helpers
static inline void for_each_pixel(FrameBufferView& fb, auto&& fn) {
  for (int y = 0; y < fb.height; y++) {
    uint8_t* row = fb.data + y * fb.stride;
    for (int x = 0; x < fb.width; x++) {
      ColorRGBA8 c{row[x * 4 + 0], row[x * 4 + 1], row[x * 4 + 2], row[x * 4 + 3]};
      fn(x, y, c);
      row[x * 4 + 0] = c.r;
      row[x * 4 + 1] = c.g;
      row[x * 4 + 2] = c.b;
      row[x * 4 + 3] = c.a;
    }
  }
}

// Movement / Dynamic variants (stubs; require sampling prev using EEL code)
void MovementEffect::init(const InitContext&) {}
std::vector<Param> MovementEffect::parameters() const { return {}; }
void MovementEffect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  // For now, copy previous to current (placeholder).
  if (!ctx.fb.previous.data) return;
  for (int y = 0; y < dst.height; y++) {
    const uint8_t* s = ctx.fb.previous.data + y * ctx.fb.previous.stride;
    uint8_t* d = dst.data + y * dst.stride;
    std::copy(s, s + dst.width * 4, d);
  }
}

void DynamicMovementEffect::init(const InitContext&) {}
std::vector<Param> DynamicMovementEffect::parameters() const { return {}; }
void DynamicMovementEffect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  MovementEffect().process(ctx, dst);
}

void DynamicDistanceModifierEffect::init(const InitContext&) {}
std::vector<Param> DynamicDistanceModifierEffect::parameters() const { return {}; }
void DynamicDistanceModifierEffect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  MovementEffect().process(ctx, dst);
}

void DynamicShiftEffect::init(const InitContext&) {}
std::vector<Param> DynamicShiftEffect::parameters() const { return {}; }
void DynamicShiftEffect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  (void)ctx;
  (void)dst;
}

void ZoomRotateEffect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  MovementEffect().process(ctx, dst);
}
std::vector<Param> ZoomRotateEffect::parameters() const { return {}; }

void MirrorEffect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  MovementEffect().process(ctx, dst);
}
std::vector<Param> MirrorEffect::parameters() const { return {}; }

// Convolution (stub)
void Convolution3x3Effect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  // No-op placeholder
  (void)ctx;
  (void)dst;
}
std::vector<Param> Convolution3x3Effect::parameters() const { return {}; }

void BlurBoxEffect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  // No-op placeholder
  (void)ctx;
  (void)dst;
}
std::vector<Param> BlurBoxEffect::parameters() const { return {}; }

// Color Map (stub)
void ColorMapEffect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  (void)ctx;
  (void)dst;
}
std::vector<Param> ColorMapEffect::parameters() const { return {}; }

// Invert
void InvertEffect::process(const ProcessContext&, FrameBufferView& dst) {
  for_each_pixel(dst, [&](int, int, ColorRGBA8& c) {
    c.r = (uint8_t)(255 - c.r);
    c.g = (uint8_t)(255 - c.g);
    c.b = (uint8_t)(255 - c.b);
  });
}

// Fadeout
std::vector<Param> FadeoutEffect::parameters() const {
  return {Param{"amount", ParamKind::Float, 0.04f, std::nullopt, std::nullopt, 0.0f, 1.0f}};
}
void FadeoutEffect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  const float dt = static_cast<float>(ctx.time.dt_seconds);
  float amt = 0.04f * std::clamp(dt * 60.0f, 0.0f, 4.0f);
  for_each_pixel(dst, [&](int, int, ColorRGBA8& c) {
    c.r = (uint8_t)(c.r * (1.0f - amt));
    c.g = (uint8_t)(c.g * (1.0f - amt));
    c.b = (uint8_t)(c.b * (1.0f - amt));
  });
}

// Bump / Interferences / Fast Brightness / Grain / Interleave (stubs)
void BumpEffect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  (void)ctx;
  (void)dst;
}
std::vector<Param> BumpEffect::parameters() const { return {}; }

void InterferencesEffect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  (void)ctx;
  (void)dst;
}
std::vector<Param> InterferencesEffect::parameters() const { return {}; }

void FastBrightnessEffect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  (void)ctx;
  (void)dst;
}
std::vector<Param> FastBrightnessEffect::parameters() const { return {}; }

void GrainEffect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  (void)ctx;
  (void)dst;
}
std::vector<Param> GrainEffect::parameters() const { return {}; }

void InterleaveEffect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  (void)ctx;
  (void)dst;
}
std::vector<Param> InterleaveEffect::parameters() const { return {}; }

}  // namespace avs
