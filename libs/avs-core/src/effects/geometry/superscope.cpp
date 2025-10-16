#include "effects/geometry/superscope.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

#include "effects/geometry/raster.hpp"

namespace avs::effects::geometry {

namespace {

constexpr int kMaxSuperscopePoints = 128 * 1024;

std::uint8_t toByte(double value) {
  double clamped = std::clamp(value, 0.0, 1.0);
  return static_cast<std::uint8_t>(clamped * 255.0 + 0.5);
}

int toPixelCoord(double normalized, int extent) {
  if (extent <= 0) return 0;
  double scaled = (normalized + 1.0) * 0.5 * static_cast<double>(extent - 1);
  return static_cast<int>(
      std::floor(std::clamp(scaled, 0.0, static_cast<double>(extent - 1)) + 0.5));
}

ColorRGBA8 sampleColor(const FrameBufferView& fb, double nx, double ny) {
  if (!fb.data || fb.width <= 0 || fb.height <= 0) return ColorRGBA8{0, 0, 0, 255};
  double sx = (nx + 1.0) * 0.5 * static_cast<double>(fb.width - 1);
  double sy = (ny + 1.0) * 0.5 * static_cast<double>(fb.height - 1);
  int ix = static_cast<int>(std::clamp(sx, 0.0, static_cast<double>(fb.width - 1)));
  int iy = static_cast<int>(std::clamp(sy, 0.0, static_cast<double>(fb.height - 1)));
  const std::uint8_t* p =
      fb.data + static_cast<std::size_t>(iy) * fb.stride + static_cast<std::size_t>(ix) * 4u;
  return ColorRGBA8{p[0], p[1], p[2], 255};
}

float computeRms(const std::vector<float>& samples) {
  if (samples.empty()) return 0.0f;
  double accum = 0.0;
  for (float v : samples) {
    accum += static_cast<double>(v) * static_cast<double>(v);
  }
  return static_cast<float>(std::sqrt(accum / static_cast<double>(samples.size())));
}

void resampleWaveform(const std::vector<float>& src, std::array<float, 576>& dst) {
  if (src.empty()) {
    dst.fill(0.0f);
    return;
  }
  for (std::size_t i = 0; i < dst.size(); ++i) {
    double t = dst.size() > 1 ? static_cast<double>(i) / static_cast<double>(dst.size() - 1) : 0.0;
    double pos = t * static_cast<double>(src.size() - 1);
    std::size_t base = static_cast<std::size_t>(std::floor(pos));
    std::size_t next = std::min(base + 1, src.size() - 1);
    double frac = pos - static_cast<double>(base);
    double v0 = src[base];
    double v1 = src[next];
    dst[i] = static_cast<float>(v0 + (v1 - v0) * frac);
  }
}

void copyLegacyChannel(const std::vector<float>& src, std::uint8_t* dst, std::size_t count,
                       bool isSpectrum) {
  if (src.empty()) {
    std::fill(dst, dst + count, 0);
    return;
  }
  for (std::size_t i = 0; i < count; ++i) {
    double t = count > 1 ? static_cast<double>(i) / static_cast<double>(count - 1) : 0.0;
    double pos = t * static_cast<double>(src.size() - 1);
    std::size_t base = static_cast<std::size_t>(std::floor(pos));
    std::size_t next = std::min(base + 1, src.size() - 1);
    double frac = pos - static_cast<double>(base);
    double v0 = src[base];
    double v1 = src[next];
    double value = v0 + (v1 - v0) * frac;
    if (isSpectrum) {
      value = std::clamp(value, 0.0, 1.0);
      dst[i] = static_cast<std::uint8_t>(std::lround(value * 255.0));
    } else {
      value = std::clamp(value, -1.0, 1.0);
      dst[i] = static_cast<std::uint8_t>(std::lround(value * 127.5 + 127.5));
    }
  }
}

}  // namespace

SuperscopeRuntime::SuperscopeRuntime() = default;

SuperscopeRuntime::~SuperscopeRuntime() {
  if (initCode_) vm_.freeCode(initCode_);
  if (frameCode_) vm_.freeCode(frameCode_);
  if (beatCode_) vm_.freeCode(beatCode_);
  if (pointCode_) vm_.freeCode(pointCode_);
}

void SuperscopeRuntime::setScripts(const SuperscopeConfig& config) {
  config_ = config;
  dirty_ = true;
  pendingBeat_ = false;
}

void SuperscopeRuntime::setOverrides(std::optional<int> points, std::optional<float> thickness,
                                     std::optional<bool> lineMode) {
  overridePoints_ = points;
  overrideThickness_ = thickness;
  overrideLineMode_ = lineMode;
}

void SuperscopeRuntime::init(const InitContext& ctx) {
  width_ = ctx.frame_size.w;
  height_ = ctx.frame_size.h;
  time_ = vm_.regVar("time");
  frame_ = vm_.regVar("frame");
  bass_ = vm_.regVar("bass");
  mid_ = vm_.regVar("mid");
  treb_ = vm_.regVar("treb");
  rms_ = vm_.regVar("rms");
  beat_ = vm_.regVar("beat");
  bVar_ = vm_.regVar("b");
  n_ = vm_.regVar("n");
  i_ = vm_.regVar("i");
  v_ = vm_.regVar("v");
  wVar_ = vm_.regVar("w");
  hVar_ = vm_.regVar("h");
  skip_ = vm_.regVar("skip");
  lineSize_ = vm_.regVar("linesize");
  drawMode_ = vm_.regVar("drawmode");
  x_ = vm_.regVar("x");
  y_ = vm_.regVar("y");
  r_ = vm_.regVar("red");
  g_ = vm_.regVar("green");
  b_ = vm_.regVar("blue");
  if (wVar_) *wVar_ = static_cast<EEL_F>(width_);
  if (hVar_) *hVar_ = static_cast<EEL_F>(height_);
  if (n_) *n_ = 100.0f;
  if (skip_) *skip_ = 0.0f;
  if (lineSize_) *lineSize_ = 1.0f;
  if (drawMode_) *drawMode_ = 0.0f;
  if (bVar_) *bVar_ = 0.0f;
  if (r_) *r_ = 0.0f;
  if (g_) *g_ = 0.0f;
  if (b_) *b_ = 0.0f;
  initRan_ = false;
}

void SuperscopeRuntime::compile() {
  if (!dirty_) return;
  if (initCode_) {
    vm_.freeCode(initCode_);
    initCode_ = nullptr;
  }
  if (frameCode_) {
    vm_.freeCode(frameCode_);
    frameCode_ = nullptr;
  }
  if (beatCode_) {
    vm_.freeCode(beatCode_);
    beatCode_ = nullptr;
  }
  if (pointCode_) {
    vm_.freeCode(pointCode_);
    pointCode_ = nullptr;
  }
  if (!config_.initScript.empty()) initCode_ = vm_.compile(config_.initScript);
  if (!config_.frameScript.empty()) frameCode_ = vm_.compile(config_.frameScript);
  if (!config_.beatScript.empty()) beatCode_ = vm_.compile(config_.beatScript);
  if (!config_.pointScript.empty()) pointCode_ = vm_.compile(config_.pointScript);
  dirty_ = false;
  initRan_ = false;
}

void SuperscopeRuntime::update(const ProcessContext& ctx) {
  if (time_) *time_ = static_cast<EEL_F>(ctx.time.t_seconds);
  if (frame_) *frame_ = static_cast<EEL_F>(ctx.time.frame_index);
  if (bass_) *bass_ = ctx.audio.bass;
  if (mid_) *mid_ = ctx.audio.mid;
  if (treb_) *treb_ = ctx.audio.treb;
  float rmsValue = computeRms(ctx.audio.oscL.empty() ? ctx.audio.oscR : ctx.audio.oscL);
  if (rms_) *rms_ = rmsValue;
  bool beatFlag = ctx.audio.beat;
  if (beat_) *beat_ = beatFlag ? 1.0f : 0.0f;
  if (bVar_) *bVar_ = beatFlag ? 1.0f : 0.0f;
  if (beatFlag && !pendingBeat_) pendingBeat_ = true;
  lastRms_ = rmsValue;

  resampleWaveform(ctx.audio.oscL.empty() ? ctx.audio.oscR : ctx.audio.oscL, waveform_);

  legacyChannels_ = 0;
  const std::size_t sampleCount = EelVm::kLegacyVisSamples;
  copyLegacyChannel(ctx.audio.oscL, legacyOsc_.data(), sampleCount, false);
  if (!ctx.audio.oscR.empty()) {
    copyLegacyChannel(ctx.audio.oscR, legacyOsc_.data() + sampleCount, sampleCount, false);
    legacyChannels_ = 2;
  } else {
    std::copy_n(legacyOsc_.data(), sampleCount, legacyOsc_.data() + sampleCount);
    legacyChannels_ = 1;
  }
  copyLegacyChannel(ctx.audio.spectrum.left, legacySpec_.data(), sampleCount, true);
  if (!ctx.audio.spectrum.right.empty()) {
    copyLegacyChannel(ctx.audio.spectrum.right, legacySpec_.data() + sampleCount, sampleCount,
                      true);
  } else {
    std::copy_n(legacySpec_.data(), sampleCount, legacySpec_.data() + sampleCount);
  }

  EelVm::LegacySources sources{};
  sources.oscBase = legacyOsc_.data();
  sources.specBase = legacySpec_.data();
  sources.sampleCount = sampleCount;
  sources.channels = legacyChannels_;
  sources.audioTimeSeconds = ctx.time.t_seconds;
  sources.engineTimeSeconds = ctx.time.t_seconds;
  vm_.setLegacySources(sources);
}

void SuperscopeRuntime::render(const ProcessContext& ctx, FrameBufferView& dst) {
  compile();
  if (ctx.fb.previous.data) {
    copyFrom(dst, ctx.fb.previous);
  }
  if (!initRan_) {
    if (initCode_) vm_.execute(initCode_);
    initRan_ = true;
  }
  if (frameCode_) vm_.execute(frameCode_);
  if (pendingBeat_) {
    if (beatCode_) vm_.execute(beatCode_);
    pendingBeat_ = false;
  }

  if (overridePoints_ && n_) {
    *n_ = static_cast<EEL_F>(*overridePoints_);
  }
  if (overrideThickness_ && lineSize_) {
    *lineSize_ = static_cast<EEL_F>(*overrideThickness_);
  }
  if (overrideLineMode_ && drawMode_) {
    *drawMode_ = *overrideLineMode_ ? 1.0f : 0.0f;
  }

  if (wVar_) *wVar_ = static_cast<EEL_F>(width_);
  if (hVar_) *hVar_ = static_cast<EEL_F>(height_);

  int total = n_ ? static_cast<int>(*n_) : 0;
  if (total <= 0) total = 512;
  total = std::clamp(total, 1, kMaxSuperscopePoints);
  if (n_) *n_ = static_cast<EEL_F>(total);

  bool haveLast = false;
  int lastX = 0;
  int lastY = 0;
  FrameBufferView prev = ctx.fb.previous;
  for (int idx = 0; idx < total; ++idx) {
    double normIndex = total > 1 ? static_cast<double>(idx) / static_cast<double>(total - 1) : 0.0;
    if (i_) *i_ = static_cast<EEL_F>(normIndex);
    if (v_) {
      double pos = normIndex * static_cast<double>(waveform_.size() - 1);
      std::size_t base = static_cast<std::size_t>(std::floor(pos));
      std::size_t next = std::min(base + 1, waveform_.size() - 1);
      double frac = pos - static_cast<double>(base);
      double value = waveform_[base] + (waveform_[next] - waveform_[base]) * frac;
      *v_ = static_cast<EEL_F>(value);
    }
    if (skip_) *skip_ = 0.0f;

    double defaultX = normIndex * 2.0 - 1.0;
    double defaultY = 0.0;
    if (x_) *x_ = static_cast<EEL_F>(defaultX);
    if (y_) *y_ = static_cast<EEL_F>(defaultY);

    ColorRGBA8 baseColor = sampleColor(prev.data ? prev : dst, defaultX, defaultY);
    if (r_) *r_ = static_cast<EEL_F>(baseColor.r / 255.0f);
    if (g_) *g_ = static_cast<EEL_F>(baseColor.g / 255.0f);
    if (b_) *b_ = static_cast<EEL_F>(baseColor.b / 255.0f);

    if (pointCode_) vm_.execute(pointCode_);

    double xNorm = x_ ? static_cast<double>(*x_) : defaultX;
    double yNorm = y_ ? static_cast<double>(*y_) : defaultY;
    ColorRGBA8 sampledColor = sampleColor(prev.data ? prev : dst, xNorm, yNorm);
    double red = r_ ? static_cast<double>(*r_) : sampledColor.r / 255.0;
    double green = g_ ? static_cast<double>(*g_) : sampledColor.g / 255.0;
    double blue = b_ ? static_cast<double>(*b_) : sampledColor.b / 255.0;
    ColorRGBA8 color{toByte(red), toByte(green), toByte(blue), 255};
    int px = toPixelCoord(xNorm, width_);
    int py = toPixelCoord(yNorm, height_);

    int thickness = 1;
    if (lineSize_) {
      int lw = static_cast<int>(std::floor(*lineSize_ + 0.5f));
      thickness = std::clamp(lw, 1, 255);
    }
    double skipValue = skip_ ? static_cast<double>(*skip_) : 0.0;
    bool lineMode = drawMode_ ? *drawMode_ > 0.5f : false;
    if (skipValue <= 0.0) {
      if (lineMode && haveLast) {
        drawThickLine(dst, lastX, lastY, px, py, thickness, color);
      } else {
        drawThickLine(dst, px, py, px, py, thickness, color);
      }
    }
    haveLast = true;
    lastX = px;
    lastY = py;
  }
}

}  // namespace avs::effects::geometry
