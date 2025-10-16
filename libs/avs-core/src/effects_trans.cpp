#include "avs/effects_trans.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

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

std::string toLowerCopy(std::string_view text) {
  std::string result(text);
  std::transform(result.begin(), result.end(), result.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return result;
}

int valueToInt(const ParamValue& value, int fallback) {
  if (std::holds_alternative<int>(value)) {
    return std::get<int>(value);
  }
  if (std::holds_alternative<float>(value)) {
    return static_cast<int>(std::lround(std::get<float>(value)));
  }
  if (std::holds_alternative<std::string>(value)) {
    const std::string& text = std::get<std::string>(value);
    try {
      size_t consumed = 0;
      int parsed = std::stoi(text, &consumed, 10);
      if (consumed == text.size()) {
        return parsed;
      }
    } catch (...) {
      return fallback;
    }
  }
  return fallback;
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

std::vector<Param> MirrorEffect::parameters() const {
  Param mode{"mode", ParamKind::Select, static_cast<int>(mode_)};
  mode.options = {
      {"horizontal", "Horizontal"},
      {"vertical", "Vertical"},
      {"quad", "Quad"},
  };
  return {mode};
}

void MirrorEffect::set_parameter(std::string_view name, const ParamValue& value) {
  if (name != "mode") return;
  Mode newMode = mode_;
  if (std::holds_alternative<int>(value)) {
    int idx = std::clamp(std::get<int>(value), 0, 2);
    newMode = static_cast<Mode>(idx);
  } else if (std::holds_alternative<float>(value)) {
    int idx = std::clamp(static_cast<int>(std::lround(std::get<float>(value))), 0, 2);
    newMode = static_cast<Mode>(idx);
  } else if (std::holds_alternative<std::string>(value)) {
    std::string lowered = toLowerCopy(std::get<std::string>(value));
    if (lowered == "horizontal" || lowered == "h") {
      newMode = Mode::Horizontal;
    } else if (lowered == "vertical" || lowered == "v") {
      newMode = Mode::Vertical;
    } else if (lowered == "quad" || lowered == "both" || lowered == "hv") {
      newMode = Mode::Quad;
    }
  }
  mode_ = newMode;
}

void MirrorEffect::process(const ProcessContext&, FrameBufferView& dst) {
  if (!dst.data || dst.width <= 0 || dst.height <= 0) {
    return;
  }
  const int stride = dst.stride;
  const std::size_t frameBytes = static_cast<std::size_t>(dst.height) * stride;
  std::vector<std::uint8_t> copy(frameBytes);
  for (int y = 0; y < dst.height; ++y) {
    std::memcpy(copy.data() + static_cast<std::size_t>(y) * stride,
                dst.data + static_cast<std::size_t>(y) * stride, stride);
  }

  for (int y = 0; y < dst.height; ++y) {
    for (int x = 0; x < dst.width; ++x) {
      int srcX = x;
      int srcY = y;
      switch (mode_) {
        case Mode::Horizontal:
          srcX = dst.width - 1 - x;
          break;
        case Mode::Vertical:
          srcY = dst.height - 1 - y;
          break;
        case Mode::Quad:
          srcX = (x < dst.width / 2) ? x : dst.width - 1 - x;
          srcY = (y < dst.height / 2) ? y : dst.height - 1 - y;
          break;
      }
      srcX = std::clamp(srcX, 0, dst.width - 1);
      srcY = std::clamp(srcY, 0, dst.height - 1);
      const std::uint8_t* src = copy.data() + static_cast<std::size_t>(srcY) * stride + srcX * 4;
      std::uint8_t* out = dst.data + static_cast<std::size_t>(y) * stride + x * 4;
      std::copy(src, src + 4, out);
    }
  }
}

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

std::vector<Param> InterleaveEffect::parameters() const {
  Param count{"frame_count", ParamKind::Int, frameCount_};
  count.i_min = 1;
  Param offset{"offset", ParamKind::Int, offset_};
  return {count, offset};
}

void InterleaveEffect::set_parameter(std::string_view name, const ParamValue& value) {
  if (name == "frame_count") {
    int newCount = std::max(1, valueToInt(value, frameCount_));
    if (newCount != frameCount_) {
      frameCount_ = newCount;
      width_ = height_ = stride_ = 0;
      frames_.clear();
      ready_.clear();
    }
  } else if (name == "offset") {
    offset_ = valueToInt(value, offset_);
  }
}

void InterleaveEffect::ensureHistory(int width, int height, int stride) {
  if (width <= 0 || height <= 0 || stride <= 0) {
    frames_.clear();
    ready_.clear();
    width_ = height_ = stride_ = 0;
    return;
  }
  frameCount_ = std::max(frameCount_, 1);
  const std::size_t frameBytes = static_cast<std::size_t>(height) * stride;
  if (width != width_ || height != height_ || stride != stride_ ||
      static_cast<int>(frames_.size()) != frameCount_) {
    width_ = width;
    height_ = height;
    stride_ = stride;
    frames_.assign(frameCount_, std::vector<std::uint8_t>(frameBytes, 0));
    ready_.assign(frameCount_, false);
    return;
  }
  for (auto& frame : frames_) {
    if (frame.size() != frameBytes) {
      frame.assign(frameBytes, 0);
    }
  }
  if (static_cast<int>(ready_.size()) != frameCount_) {
    ready_.assign(frameCount_, false);
  }
}

void InterleaveEffect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  if (!dst.data || dst.width <= 0 || dst.height <= 0) {
    return;
  }
  ensureHistory(dst.width, dst.height, dst.stride);
  if (frames_.empty()) {
    return;
  }

  const std::size_t frameBytes = static_cast<std::size_t>(dst.height) * dst.stride;
  int fc = std::max(frameCount_, 1);
  int frameIndex = ctx.time.frame_index;
  int current = frameIndex % fc;
  if (current < 0) {
    current += fc;
  }

  std::memcpy(frames_[current].data(), dst.data, frameBytes);
  ready_[current] = true;

  if (fc <= 1) {
    return;
  }

  const int base = ((frameIndex + offset_) % fc + fc) % fc;
  for (int y = 0; y < dst.height; ++y) {
    for (int x = 0; x < dst.width; ++x) {
      int index = (base + ((x + y) % fc)) % fc;
      if (index < 0) index += fc;
      if (index >= fc) index %= fc;
      if (index < 0 || index >= fc || !ready_[index]) {
        index = current;
      }
      const std::uint8_t* src = frames_[index].data() + static_cast<std::size_t>(y) * stride_ + x * 4;
      std::uint8_t* out = dst.data + static_cast<std::size_t>(y) * dst.stride + x * 4;
      std::copy(src, src + 4, out);
    }
  }
}

}  // namespace avs
