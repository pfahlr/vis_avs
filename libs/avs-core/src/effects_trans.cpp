#include "avs/effects_trans.hpp"
#include "avs/core.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>

namespace avs {

namespace {

float computeAudioLevel(const AudioFeatures& audio) {
  const auto accumulateChannel = [](const std::vector<float>& samples) {
    if (samples.empty()) return 0.0f;
    float sum = 0.0f;
    for (float v : samples) {
      sum += std::fabs(v);
    }
    return sum / static_cast<float>(samples.size());
  };

  float oscLevel = std::max(accumulateChannel(audio.oscL), accumulateChannel(audio.oscR));
  float bandLevel = std::max({std::fabs(audio.bass), std::fabs(audio.mid), std::fabs(audio.treb)});
  return std::clamp(std::max(oscLevel, bandLevel), 0.0f, 1.0f);
}

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

Param makeIntParam(std::string name, int value, int min, int max) {
  Param p;
  p.name = std::move(name);
  p.kind = ParamKind::Int;
  p.value = value;
  p.i_min = min;
  p.i_max = max;
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

void MovementEffect::init(const InitContext&) {}

std::vector<Param> MovementEffect::parameters() const {
  std::vector<Param> params;
  params.push_back(makeBoolParam("wrap", false));
  params.push_back(makeFloatParam("speed", 0.6f, 0.0f, 2.0f));
  return params;
}

void MovementEffect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  if (!ctx.fb.previous.data || !dst.data) return;
  const float t = static_cast<float>(ctx.time.t_seconds);
  const float dx = std::sin(t * 0.45f) * 12.0f;
  const float dy = std::cos(t * 0.33f) * 8.0f;

  SampleOptions opt;
  opt.filter = Filter::Bilinear;
  opt.wrap = Wrap::Wrap;
  for (int y = 0; y < dst.height; ++y) {
    uint8_t* row = dst.data + static_cast<size_t>(y) * dst.stride;
    for (int x = 0; x < dst.width; ++x) {
      ColorRGBA8 sample = sampleRGBA(ctx.fb.previous,
                                     static_cast<float>(x) - dx,
                                     static_cast<float>(y) - dy,
                                     opt);
      row[x * 4 + 0] = sample.r;
      row[x * 4 + 1] = sample.g;
      row[x * 4 + 2] = sample.b;
      row[x * 4 + 3] = sample.a;
    }
  }
}

void DynamicMovementEffect::init(const InitContext&) {}

std::vector<Param> DynamicMovementEffect::parameters() const {
  std::vector<Param> params;
  params.push_back(makeFloatParam("strength", 1.0f, 0.0f, 4.0f));
  return params;
}

void DynamicMovementEffect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  if (!ctx.fb.previous.data || !dst.data) return;
  const float level = computeAudioLevel(ctx.audio);
  const float t = static_cast<float>(ctx.time.t_seconds);
  const float dx = std::sin(t * 1.2f) * (6.0f + level * 24.0f);
  const float dy = std::cos(t * 0.9f) * (6.0f + level * 18.0f);

  SampleOptions opt;
  opt.filter = Filter::Bilinear;
  opt.wrap = Wrap::Wrap;
  for (int y = 0; y < dst.height; ++y) {
    uint8_t* row = dst.data + static_cast<size_t>(y) * dst.stride;
    for (int x = 0; x < dst.width; ++x) {
      ColorRGBA8 sample = sampleRGBA(ctx.fb.previous,
                                     static_cast<float>(x) - dx * (0.5f + y / std::max(1, dst.height)),
                                     static_cast<float>(y) - dy,
                                     opt);
      row[x * 4 + 0] = sample.r;
      row[x * 4 + 1] = sample.g;
      row[x * 4 + 2] = sample.b;
      row[x * 4 + 3] = sample.a;
    }
  }
}

void DynamicDistanceModifierEffect::init(const InitContext&) {}

std::vector<Param> DynamicDistanceModifierEffect::parameters() const {
  std::vector<Param> params;
  params.push_back(makeFloatParam("strength", 0.5f, 0.0f, 2.0f));
  return params;
}

void DynamicDistanceModifierEffect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  if (!ctx.fb.previous.data || !dst.data) return;
  const float level = computeAudioLevel(ctx.audio);
  SampleOptions opt;
  opt.filter = Filter::Bilinear;
  opt.wrap = Wrap::Clamp;

  const float cx = (dst.width - 1) * 0.5f;
  const float cy = (dst.height - 1) * 0.5f;
  for (int y = 0; y < dst.height; ++y) {
    uint8_t* row = dst.data + static_cast<size_t>(y) * dst.stride;
    for (int x = 0; x < dst.width; ++x) {
      float ox = static_cast<float>(x) - cx;
      float oy = static_cast<float>(y) - cy;
      float radius = std::sqrt(ox * ox + oy * oy);
      float angle = std::atan2(oy, ox);
      float warp = 1.0f + level * 0.6f;
      float warpedRadius = radius * warp;
      float sampleX = cx + std::cos(angle) * warpedRadius;
      float sampleY = cy + std::sin(angle) * warpedRadius;
      ColorRGBA8 sample = sampleRGBA(ctx.fb.previous, sampleX, sampleY, opt);
      row[x * 4 + 0] = sample.r;
      row[x * 4 + 1] = sample.g;
      row[x * 4 + 2] = sample.b;
      row[x * 4 + 3] = sample.a;
    }
  }
}

void DynamicShiftEffect::init(const InitContext&) {}

std::vector<Param> DynamicShiftEffect::parameters() const {
  std::vector<Param> params;
  params.push_back(makeFloatParam("amplitude", 0.5f, 0.0f, 2.0f));
  return params;
}

void DynamicShiftEffect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  if (!ctx.fb.previous.data || !dst.data) return;
  const float level = computeAudioLevel(ctx.audio);
  SampleOptions opt;
  opt.filter = Filter::Bilinear;
  opt.wrap = Wrap::Wrap;

  const float base = 8.0f + level * 24.0f;
  for (int y = 0; y < dst.height; ++y) {
    uint8_t* row = dst.data + static_cast<size_t>(y) * dst.stride;
    float phase = static_cast<float>(y) / std::max(1, dst.height - 1);
    float offset = std::sin(phase * 6.28318f + static_cast<float>(ctx.time.t_seconds) * 1.5f) * base;
    for (int x = 0; x < dst.width; ++x) {
      ColorRGBA8 sample = sampleRGBA(ctx.fb.previous,
                                     static_cast<float>(x) - offset,
                                     static_cast<float>(y),
                                     opt);
      row[x * 4 + 0] = sample.r;
      row[x * 4 + 1] = sample.g;
      row[x * 4 + 2] = sample.b;
      row[x * 4 + 3] = sample.a;
    }
  }
}

std::vector<Param> ZoomRotateEffect::parameters() const {
  std::vector<Param> params;
  params.push_back(makeFloatParam("zoom", 1.0f, 0.1f, 4.0f));
  params.push_back(makeFloatParam("angle", 0.0f, -3.14159f, 3.14159f));
  return params;
}

void ZoomRotateEffect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  if (!ctx.fb.previous.data || !dst.data) return;
  const float time = static_cast<float>(ctx.time.t_seconds);
  const float zoom = 1.0f + 0.1f * std::sin(time * 0.7f);
  const float angle = 0.1f * std::sin(time * 0.4f);

  const float cosA = std::cos(angle);
  const float sinA = std::sin(angle);
  SampleOptions opt;
  opt.filter = Filter::Bilinear;
  opt.wrap = Wrap::Wrap;

  const float cx = (dst.width - 1) * 0.5f;
  const float cy = (dst.height - 1) * 0.5f;
  for (int y = 0; y < dst.height; ++y) {
    uint8_t* row = dst.data + static_cast<size_t>(y) * dst.stride;
    for (int x = 0; x < dst.width; ++x) {
      float dx = (static_cast<float>(x) - cx) / zoom;
      float dy = (static_cast<float>(y) - cy) / zoom;
      float sx = dx * cosA - dy * sinA + cx;
      float sy = dx * sinA + dy * cosA + cy;
      ColorRGBA8 sample = sampleRGBA(ctx.fb.previous, sx, sy, opt);
      row[x * 4 + 0] = sample.r;
      row[x * 4 + 1] = sample.g;
      row[x * 4 + 2] = sample.b;
      row[x * 4 + 3] = sample.a;
    }
  }
}

std::vector<Param> MirrorEffect::parameters() const {
  std::vector<Param> params;
  params.push_back(makeBoolParam("vertical", true));
  return params;
}

void MirrorEffect::process(const ProcessContext&, FrameBufferView& dst) {
  if (!dst.data) return;
  for (int y = 0; y < dst.height; ++y) {
    uint8_t* row = dst.data + static_cast<size_t>(y) * dst.stride;
    for (int x = 0; x < dst.width / 2; ++x) {
      int mirror = dst.width - 1 - x;
      uint8_t* left = row + static_cast<size_t>(x) * 4u;
      uint8_t* right = row + static_cast<size_t>(mirror) * 4u;
      right[0] = left[0];
      right[1] = left[1];
      right[2] = left[2];
      right[3] = left[3];
    }
  }
}

std::vector<Param> Convolution3x3Effect::parameters() const {
  std::vector<Param> params;
  params.push_back(makeSelectParam("kernel",
                                   "Sharpen",
                                   {OptionItem{"sharpen", "Sharpen"},
                                    OptionItem{"edge", "Edge"}}));
  return params;
}

void Convolution3x3Effect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  const FrameBufferView& src = ctx.fb.previous.data ? ctx.fb.previous : dst;
  if (!src.data || !dst.data) return;

  const int kernels[2][9] = {
      {0, -1, 0, -1, 5, -1, 0, -1, 0},
      {-1, -1, -1, -1, 8, -1, -1, -1, -1},
  };
  const int* kernel = kernels[0];
  const int kernelWeight = 1;

  SampleOptions opt;
  opt.filter = Filter::Nearest;
  opt.wrap = Wrap::Clamp;

  for (int y = 0; y < dst.height; ++y) {
    uint8_t* row = dst.data + static_cast<size_t>(y) * dst.stride;
    for (int x = 0; x < dst.width; ++x) {
      int acc[4] = {0, 0, 0, 0};
      for (int ky = -1; ky <= 1; ++ky) {
        for (int kx = -1; kx <= 1; ++kx) {
          int coeff = kernel[(ky + 1) * 3 + (kx + 1)];
          ColorRGBA8 sample = sampleRGBA(src, static_cast<float>(x + kx), static_cast<float>(y + ky), opt);
          acc[0] += coeff * sample.r;
          acc[1] += coeff * sample.g;
          acc[2] += coeff * sample.b;
          acc[3] += coeff * sample.a;
        }
      }
      row[x * 4 + 0] = static_cast<uint8_t>(std::clamp(acc[0] / kernelWeight, 0, 255));
      row[x * 4 + 1] = static_cast<uint8_t>(std::clamp(acc[1] / kernelWeight, 0, 255));
      row[x * 4 + 2] = static_cast<uint8_t>(std::clamp(acc[2] / kernelWeight, 0, 255));
      row[x * 4 + 3] = static_cast<uint8_t>(std::clamp(acc[3] / kernelWeight, 0, 255));
    }
  }
}

std::vector<Param> BlurBoxEffect::parameters() const {
  std::vector<Param> params;
  params.push_back(makeIntParam("radius", 1, 1, 4));
  return params;
}

void BlurBoxEffect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  const FrameBufferView& src = ctx.fb.previous.data ? ctx.fb.previous : dst;
  if (!src.data || !dst.data) return;

  SampleOptions opt;
  opt.filter = Filter::Nearest;
  opt.wrap = Wrap::Clamp;

  for (int y = 0; y < dst.height; ++y) {
    uint8_t* row = dst.data + static_cast<size_t>(y) * dst.stride;
    for (int x = 0; x < dst.width; ++x) {
      int acc[4] = {0, 0, 0, 0};
      int count = 0;
      for (int ky = -1; ky <= 1; ++ky) {
        for (int kx = -1; kx <= 1; ++kx) {
          ColorRGBA8 sample = sampleRGBA(src, static_cast<float>(x + kx), static_cast<float>(y + ky), opt);
          acc[0] += sample.r;
          acc[1] += sample.g;
          acc[2] += sample.b;
          acc[3] += sample.a;
          ++count;
        }
      }
      row[x * 4 + 0] = static_cast<uint8_t>(acc[0] / count);
      row[x * 4 + 1] = static_cast<uint8_t>(acc[1] / count);
      row[x * 4 + 2] = static_cast<uint8_t>(acc[2] / count);
      row[x * 4 + 3] = static_cast<uint8_t>(acc[3] / count);
    }
  }
}

std::vector<Param> ColorMapEffect::parameters() const {
  std::vector<Param> params;
  params.push_back(makeColorParam("low_color", ColorRGBA8{0, 32, 128, 255}));
  params.push_back(makeColorParam("high_color", ColorRGBA8{255, 240, 64, 255}));
  return params;
}

void ColorMapEffect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  const FrameBufferView& src = ctx.fb.previous.data ? ctx.fb.previous : dst;
  if (!src.data || !dst.data) return;

  const ColorRGBA8 low{0, 32, 128, 255};
  const ColorRGBA8 high{255, 240, 64, 255};

  for (int y = 0; y < dst.height; ++y) {
    uint8_t* row = dst.data + static_cast<size_t>(y) * dst.stride;
    for (int x = 0; x < dst.width; ++x) {
      const uint8_t* srcPx = src.data + static_cast<size_t>(y) * src.stride + static_cast<size_t>(x) * 4u;
      float gray = (srcPx[0] + srcPx[1] + srcPx[2]) / (3.0f * 255.0f);
      auto lerpChannel = [&](uint8_t a, uint8_t b) {
        return static_cast<uint8_t>(std::clamp(a + (b - a) * gray, 0.0f, 255.0f));
      };
      row[x * 4 + 0] = lerpChannel(low.r, high.r);
      row[x * 4 + 1] = lerpChannel(low.g, high.g);
      row[x * 4 + 2] = lerpChannel(low.b, high.b);
      row[x * 4 + 3] = srcPx[3];
    }
  }
}

void InvertEffect::process(const ProcessContext&, FrameBufferView& dst) {
  if (!dst.data) return;
  for (int y = 0; y < dst.height; ++y) {
    uint8_t* row = dst.data + static_cast<size_t>(y) * dst.stride;
    for (int x = 0; x < dst.width; ++x) {
      row[x * 4 + 0] = 255 - row[x * 4 + 0];
      row[x * 4 + 1] = 255 - row[x * 4 + 1];
      row[x * 4 + 2] = 255 - row[x * 4 + 2];
    }
  }
}

std::vector<Param> FadeoutEffect::parameters() const {
  std::vector<Param> params;
  params.push_back(makeFloatParam("amount", 0.04f, 0.0f, 1.0f));
  return params;
}

void FadeoutEffect::process(const ProcessContext&, FrameBufferView& dst) {
  if (!dst.data) return;
  const float amount = 0.04f;
  for (int y = 0; y < dst.height; ++y) {
    uint8_t* row = dst.data + static_cast<size_t>(y) * dst.stride;
    for (int x = 0; x < dst.width; ++x) {
      row[x * 4 + 0] = static_cast<uint8_t>(row[x * 4 + 0] * (1.0f - amount));
      row[x * 4 + 1] = static_cast<uint8_t>(row[x * 4 + 1] * (1.0f - amount));
      row[x * 4 + 2] = static_cast<uint8_t>(row[x * 4 + 2] * (1.0f - amount));
    }
  }
}

std::vector<Param> BumpEffect::parameters() const {
  std::vector<Param> params;
  params.push_back(makeFloatParam("strength", 0.4f, 0.0f, 2.0f));
  return params;
}

void BumpEffect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  if (!dst.data) return;
  const float strength = 0.4f + computeAudioLevel(ctx.audio) * 0.6f;
  const float cx = (dst.width - 1) * 0.5f;
  const float cy = (dst.height - 1) * 0.5f;
  const float radius = std::max(1.0f, std::min(dst.width, dst.height) * 0.5f);

  for (int y = 0; y < dst.height; ++y) {
    uint8_t* row = dst.data + static_cast<size_t>(y) * dst.stride;
    for (int x = 0; x < dst.width; ++x) {
      float dx = static_cast<float>(x) - cx;
      float dy = static_cast<float>(y) - cy;
      float d = std::sqrt(dx * dx + dy * dy) / radius;
      float factor = std::max(0.0f, 1.0f - d) * strength;
      row[x * 4 + 0] = static_cast<uint8_t>(std::clamp(row[x * 4 + 0] * (1.0f + factor), 0.0f, 255.0f));
      row[x * 4 + 1] = static_cast<uint8_t>(std::clamp(row[x * 4 + 1] * (1.0f + factor), 0.0f, 255.0f));
      row[x * 4 + 2] = static_cast<uint8_t>(std::clamp(row[x * 4 + 2] * (1.0f + factor), 0.0f, 255.0f));
    }
  }
}

std::vector<Param> InterferencesEffect::parameters() const {
  std::vector<Param> params;
  params.push_back(makeFloatParam("frequency", 6.0f, 1.0f, 24.0f));
  params.push_back(makeFloatParam("strength", 0.25f, 0.0f, 1.0f));
  return params;
}

void InterferencesEffect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  if (!dst.data) return;
  const float freq = 6.0f + computeAudioLevel(ctx.audio) * 12.0f;
  const float strength = 0.25f;
  for (int y = 0; y < dst.height; ++y) {
    uint8_t* row = dst.data + static_cast<size_t>(y) * dst.stride;
    float stripe = 0.5f + 0.5f * std::sin(static_cast<float>(y) * freq * 0.1f + static_cast<float>(ctx.time.t_seconds) * 3.0f);
    uint8_t delta = static_cast<uint8_t>(stripe * 255.0f * strength);
    for (int x = 0; x < dst.width; ++x) {
      row[x * 4 + 0] = static_cast<uint8_t>(std::clamp(row[x * 4 + 0] + delta, 0, 255));
      row[x * 4 + 1] = static_cast<uint8_t>(std::clamp(row[x * 4 + 1] + delta / 2, 0, 255));
      row[x * 4 + 2] = static_cast<uint8_t>(std::clamp(row[x * 4 + 2] + delta / 4, 0, 255));
    }
  }
}

std::vector<Param> FastBrightnessEffect::parameters() const {
  std::vector<Param> params;
  params.push_back(makeFloatParam("gain", 1.0f, 0.0f, 4.0f));
  return params;
}

void FastBrightnessEffect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  if (!dst.data) return;
  const float gain = 1.0f + computeAudioLevel(ctx.audio) * 0.8f;
  for (int y = 0; y < dst.height; ++y) {
    uint8_t* row = dst.data + static_cast<size_t>(y) * dst.stride;
    for (int x = 0; x < dst.width; ++x) {
      row[x * 4 + 0] = static_cast<uint8_t>(std::clamp(row[x * 4 + 0] * gain, 0.0f, 255.0f));
      row[x * 4 + 1] = static_cast<uint8_t>(std::clamp(row[x * 4 + 1] * gain, 0.0f, 255.0f));
      row[x * 4 + 2] = static_cast<uint8_t>(std::clamp(row[x * 4 + 2] * gain, 0.0f, 255.0f));
    }
  }
}

std::vector<Param> GrainEffect::parameters() const {
  std::vector<Param> params;
  params.push_back(makeFloatParam("amount", 0.2f, 0.0f, 1.0f));
  return params;
}

void GrainEffect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  if (!dst.data) return;
  std::minstd_rand rng;
  if (ctx.rng) {
    rng.seed((*ctx.rng).s[0] ^ (*ctx.rng).s[1]);
  } else {
    rng.seed(static_cast<unsigned>(ctx.time.frame_index + 12345));
  }
  std::uniform_int_distribution<int> noise(-20, 20);
  for (int y = 0; y < dst.height; ++y) {
    uint8_t* row = dst.data + static_cast<size_t>(y) * dst.stride;
    for (int x = 0; x < dst.width; ++x) {
      int n = noise(rng);
      for (int c = 0; c < 3; ++c) {
        int idx = x * 4 + c;
        row[idx] = static_cast<uint8_t>(std::clamp(static_cast<int>(row[idx]) + n, 0, 255));
      }
    }
  }
}

std::vector<Param> InterleaveEffect::parameters() const {
  std::vector<Param> params;
  params.push_back(makeBoolParam("use_previous", true));
  return params;
}

void InterleaveEffect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  if (!ctx.fb.previous.data || !dst.data) return;
  for (int y = 0; y < dst.height; ++y) {
    uint8_t* rowDst = dst.data + static_cast<size_t>(y) * dst.stride;
    const uint8_t* rowPrev = ctx.fb.previous.data + static_cast<size_t>(y) * ctx.fb.previous.stride;
    if (y % 2 == 0) {
      std::copy(rowPrev, rowPrev + static_cast<size_t>(dst.width) * 4u, rowDst);
    }
  }
}

}  // namespace avs

