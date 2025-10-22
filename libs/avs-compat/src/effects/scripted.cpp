#include <algorithm>
#include <cmath>
#include <array>
#include <cstdint>
#include <avs/effects.hpp>

namespace {

constexpr int kMaxSuperscopePoints = 128 * 1024;

struct SampledColor {
  double r = 0.0;
  double g = 0.0;
  double b = 0.0;
};

static std::uint8_t toByte(double v) {
  double clamped = std::clamp(v, 0.0, 1.0);
  return static_cast<std::uint8_t>(clamped * 255.0 + 0.5);
}

static int toSuperscopeCoord(double v, int extent) {
  if (extent <= 0) return 0;
  double scaled = (v + 1.0) * static_cast<double>(extent) * 0.5;
  return static_cast<int>(std::floor(scaled));
}

static SampledColor sampleColor(const avs::Framebuffer& fb, double x, double y) {
  SampledColor c;
  if (fb.w <= 0 || fb.h <= 0 || fb.rgba.empty()) return c;
  double sx = (x + 1.0) * 0.5 * static_cast<double>(fb.w - 1);
  double sy = (y + 1.0) * 0.5 * static_cast<double>(fb.h - 1);
  int ix = static_cast<int>(std::clamp(sx, 0.0, static_cast<double>(fb.w - 1)));
  int iy = static_cast<int>(std::clamp(sy, 0.0, static_cast<double>(fb.h - 1)));
  size_t idx = (static_cast<size_t>(iy) * fb.w + ix) * 4u;
  if (idx + 2u < fb.rgba.size()) {
    c.r = fb.rgba[idx + 0u] / 255.0;
    c.g = fb.rgba[idx + 1u] / 255.0;
    c.b = fb.rgba[idx + 2u] / 255.0;
  }
  return c;
}

static void drawPoint(std::vector<std::uint8_t>& rgba,
                      int w,
                      int h,
                      int px,
                      int py,
                      const std::array<std::uint8_t, 3>& color,
                      int thickness) {
  if (w <= 0 || h <= 0 || rgba.empty()) return;
  int half = std::max(0, thickness / 2);
  for (int dy = -half; dy <= half; ++dy) {
    for (int dx = -half; dx <= half; ++dx) {
      int tx = px + dx;
      int ty = py + dy;
      if (tx < 0 || tx >= w || ty < 0 || ty >= h) continue;
      size_t idx = (static_cast<size_t>(ty) * w + tx) * 4u;
      rgba[idx + 0u] = color[0];
      rgba[idx + 1u] = color[1];
      rgba[idx + 2u] = color[2];
      rgba[idx + 3u] = 255u;
    }
  }
}

static void drawLine(std::vector<std::uint8_t>& rgba,
                     int w,
                     int h,
                     int x0,
                     int y0,
                     int x1,
                     int y1,
                     const std::array<std::uint8_t, 3>& color,
                     int thickness) {
  int dx = std::abs(x1 - x0);
  int sx = x0 < x1 ? 1 : -1;
  int dy = -std::abs(y1 - y0);
  int sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  while (true) {
    drawPoint(rgba, w, h, x0, y0, color, thickness);
    if (x0 == x1 && y0 == y1) break;
    int e2 = err * 2;
    if (e2 >= dy) {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx) {
      err += dx;
      y0 += sy;
    }
  }
}

static double sampleWave(const std::array<float, 576>& waveform, double position) {
  if (position <= 0.0) return waveform[0];
  double maxIndex = static_cast<double>(waveform.size() - 1);
  if (position >= maxIndex) return waveform.back();
  int base = static_cast<int>(std::floor(position));
  double frac = position - static_cast<double>(base);
  double v0 = waveform[static_cast<size_t>(base)];
  double v1 = waveform[static_cast<size_t>(base + 1)];
  return v0 + (v1 - v0) * frac;
}

}  // namespace

namespace avs {

ScriptedEffect::ScriptedEffect(std::string frameScript,
                               std::string pixelScript,
                               Mode mode,
                               bool colorModRecompute)
    : mode_(mode), colorModRecompute_(colorModRecompute) {
  setAllScripts("", std::move(frameScript), "", std::move(pixelScript));
}

ScriptedEffect::ScriptedEffect(std::string initScript,
                               std::string frameScript,
                               std::string beatScript,
                               std::string pixelScript,
                               Mode mode,
                               bool colorModRecompute)
    : mode_(mode), colorModRecompute_(colorModRecompute) {
  setAllScripts(std::move(initScript),
                std::move(frameScript),
                std::move(beatScript),
                std::move(pixelScript));
}

ScriptedEffect::~ScriptedEffect() {
  if (initCode_) vm_.freeCode(initCode_);
  if (frameCode_) vm_.freeCode(frameCode_);
  if (beatCode_) vm_.freeCode(beatCode_);
  if (pixelCode_) vm_.freeCode(pixelCode_);
}

void ScriptedEffect::setScripts(std::string frameScript, std::string pixelScript) {
  setAllScripts("", std::move(frameScript), "", std::move(pixelScript));
}

void ScriptedEffect::setScripts(std::string initScript,
                                std::string frameScript,
                                std::string beatScript,
                                std::string pixelScript) {
  setAllScripts(std::move(initScript),
                std::move(frameScript),
                std::move(beatScript),
                std::move(pixelScript));
}

void ScriptedEffect::setAllScripts(std::string initScript,
                                   std::string frameScript,
                                   std::string beatScript,
                                   std::string pixelScript) {
  initScript_ = std::move(initScript);
  frameScript_ = std::move(frameScript);
  beatScript_ = std::move(beatScript);
  pixelScript_ = std::move(pixelScript);
  dirty_ = true;
  pendingBeat_ = false;
  colorLutDirty_ = true;
}

void ScriptedEffect::init(int w, int h) {
  w_ = w;
  h_ = h;
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
  linesize_ = vm_.regVar("linesize");
  drawmode_ = vm_.regVar("drawmode");
  x_ = vm_.regVar("x");
  y_ = vm_.regVar("y");
  r_ = vm_.regVar("red");
  g_ = vm_.regVar("green");
  b_ = vm_.regVar("blue");
  if (wVar_) *wVar_ = static_cast<EEL_F>(w_);
  if (hVar_) *hVar_ = static_cast<EEL_F>(h_);
  if (n_) *n_ = 100.0f;
  if (skip_) *skip_ = 0.0f;
  if (linesize_) *linesize_ = 1.0f;
  if (drawmode_) *drawmode_ = 0.0f;
  if (bVar_) *bVar_ = 0.0f;
  if (r_) *r_ = 0.0f;
  if (g_) *g_ = 0.0f;
  if (b_) *b_ = 0.0f;
  lastRms_ = 0.0f;
  initRan_ = false;
  pendingBeat_ = false;
  std::fill(legacyOsc_.begin(), legacyOsc_.end(), 0);
  std::fill(legacySpec_.begin(), legacySpec_.end(), 0);
  legacyChannels_ = 0;
  EelVm::LegacySources sources{};
  sources.oscBase = legacyOsc_.data();
  sources.specBase = legacySpec_.data();
  sources.sampleCount = AudioState::kLegacyVisSamples;
  sources.channels = legacyChannels_;
  vm_.setLegacySources(sources);
  colorLutDirty_ = true;
  waveform_.fill(0.0f);

}

void ScriptedEffect::update(float time,
                            int frame,
                            const AudioState& audio,
                            const MouseState& mouse) {
  if (time_) *time_ = time;
  if (frame_) *frame_ = frame;
  if (bass_) *bass_ = audio.bands[0];
  if (mid_) *mid_ = audio.bands[1];
  if (treb_) *treb_ = audio.bands[2];
  if (rms_) *rms_ = audio.rms;
  const float threshold = 0.6f;
  bool isBeat = audio.rms > threshold && lastRms_ <= threshold;
  if (beat_) *beat_ = isBeat ? 1.0f : 0.0f;
  if (bVar_) *bVar_ = isBeat ? 1.0f : 0.0f;
  pendingBeat_ = pendingBeat_ || isBeat;
  isBeatFrame_ = isBeat;
  lastRms_ = audio.rms;

  if (!audio.spectrum.empty()) {
    size_t sourceCount = audio.spectrum.size();
    for (size_t i = 0; i < waveform_.size(); ++i) {
      double pos = 0.0;
      if (waveform_.size() > 1) {
        pos = static_cast<double>(i) *
              (sourceCount > 1 ? static_cast<double>(sourceCount - 1) / (waveform_.size() - 1) : 0.0);
      }
      size_t base = static_cast<size_t>(std::clamp(pos, 0.0, static_cast<double>(sourceCount - 1)));
      size_t next = std::min(base + 1, sourceCount - 1);
      double frac = pos - static_cast<double>(base);
      double v0 = audio.spectrum[base];
      double v1 = audio.spectrum[next];
      waveform_[i] = static_cast<float>(((v0 + (v1 - v0) * frac) * 2.0) - 1.0);
    }
  } else {
    waveform_.fill(0.0f);
  }

  legacyChannels_ = std::clamp(audio.channels, 0, 2);
  const size_t sampleCount = AudioState::kLegacyVisSamples;
  for (int ch = 0; ch < 2; ++ch) {
    std::uint8_t* oscDst = legacyOsc_.data() + static_cast<size_t>(ch) * sampleCount;
    std::uint8_t* specDst = legacySpec_.data() + static_cast<size_t>(ch) * sampleCount;
    std::fill(oscDst, oscDst + sampleCount, 0);
    std::fill(specDst, specDst + sampleCount, 0);
    if (ch < legacyChannels_) {
      for (size_t i = 0; i < sampleCount; ++i) {
        float oscVal = audio.oscilloscope[ch][i];
        float specVal = audio.spectrumLegacy[ch][i];
        oscVal = std::clamp(oscVal, -1.0f, 1.0f);
        specVal = std::clamp(specVal, 0.0f, 1.0f);
        oscDst[i] = static_cast<std::uint8_t>(std::lround(oscVal * 127.5f + 127.5f));
        specDst[i] = static_cast<std::uint8_t>(std::lround(specVal * 255.0f));
      }
    }
  }
  if (legacyChannels_ <= 1) {
    std::copy_n(legacyOsc_.data(), sampleCount, legacyOsc_.data() + sampleCount);
    std::copy_n(legacySpec_.data(), sampleCount, legacySpec_.data() + sampleCount);
  }

  EelVm::LegacySources sources{};
  sources.oscBase = legacyOsc_.data();
  sources.specBase = legacySpec_.data();
  sources.sampleCount = sampleCount;
  sources.channels = legacyChannels_;
  sources.audioTimeSeconds = audio.timeSeconds;
  sources.engineTimeSeconds = time;
  sources.mouse = mouse;
  vm_.setLegacySources(sources);
}

void ScriptedEffect::compile() {
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
  if (pixelCode_) {
    vm_.freeCode(pixelCode_);
    pixelCode_ = nullptr;
  }
  if (!initScript_.empty()) initCode_ = vm_.compile(initScript_);
  if (!frameScript_.empty()) frameCode_ = vm_.compile(frameScript_);
  if (!beatScript_.empty()) beatCode_ = vm_.compile(beatScript_);
  if (!pixelScript_.empty()) pixelCode_ = vm_.compile(pixelScript_);
  dirty_ = false;
  initRan_ = false;
  colorLutDirty_ = true;
}

void ScriptedEffect::process(const Framebuffer& in, Framebuffer& out) {
  compile();

  const size_t expectedSize = static_cast<size_t>(w_) * static_cast<size_t>(h_) * 4u;
  out.w = w_;
  out.h = h_;
  out.rgba.resize(expectedSize);
  if (in.rgba.size() == expectedSize) {
    out.rgba = in.rgba;
  } else {
    std::fill(out.rgba.begin(), out.rgba.end(), 0);
    if (!in.rgba.empty()) {
      std::copy_n(in.rgba.begin(), std::min(in.rgba.size(), out.rgba.size()), out.rgba.begin());
    }
  }

  if (!initRan_) {
    if (initCode_) vm_.execute(initCode_);
    initRan_ = true;
    colorLutDirty_ = true;
  }
  if (frameCode_) vm_.execute(frameCode_);
  bool beatExecuted = false;
  if (pendingBeat_) {
    if (beatCode_) vm_.execute(beatCode_);
    pendingBeat_ = false;
    beatExecuted = true;
  }

  if (mode_ == Mode::kColorModifier) {
    if (beatExecuted) colorLutDirty_ = true;
    if (colorModRecompute_ || colorLutDirty_) {
      for (size_t i = 0; i < 256; ++i) {
        double value = static_cast<double>(i) / 255.0;
        if (r_) *r_ = static_cast<EEL_F>(value);
        if (g_) *g_ = static_cast<EEL_F>(value);
        if (b_) *b_ = static_cast<EEL_F>(value);
        if (pixelCode_) vm_.execute(pixelCode_);
        colorLut_[0 * 256 + i] = toByte(r_ ? *r_ : value);
        colorLut_[1 * 256 + i] = toByte(g_ ? *g_ : value);
        colorLut_[2 * 256 + i] = toByte(b_ ? *b_ : value);
      }
      colorLutDirty_ = false;
    }
    for (size_t i = 0; i + 3 < out.rgba.size(); i += 4) {
      std::uint8_t r = out.rgba[i + 0];
      std::uint8_t g = out.rgba[i + 1];
      std::uint8_t b = out.rgba[i + 2];
      out.rgba[i + 0] = colorLut_[0 * 256 + r];
      out.rgba[i + 1] = colorLut_[1 * 256 + g];
      out.rgba[i + 2] = colorLut_[2 * 256 + b];
      out.rgba[i + 3] = 255u;
    }
    return;
  }

  if (wVar_) *wVar_ = static_cast<EEL_F>(w_);
  if (hVar_) *hVar_ = static_cast<EEL_F>(h_);

  int total = n_ ? static_cast<int>(*n_) : 0;
  if (total <= 0) return;
  if (total > kMaxSuperscopePoints) total = kMaxSuperscopePoints;
  if (n_) *n_ = static_cast<EEL_F>(total);

  bool haveLast = false;
  int lastX = 0;
  int lastY = 0;
  for (int idx = 0; idx < total; ++idx) {
    double normIndex = total > 1 ? static_cast<double>(idx) / static_cast<double>(total - 1) : 0.0;
    if (i_) *i_ = static_cast<EEL_F>(normIndex);
    if (n_) *n_ = static_cast<EEL_F>(total);
    if (v_) {
      double wavePos = normIndex * static_cast<double>(waveform_.size() - 1);
      *v_ = static_cast<EEL_F>(sampleWave(waveform_, wavePos));
    }
    if (skip_) *skip_ = 0.0f;

    double defaultX = normIndex * 2.0 - 1.0;
    double defaultY = 0.0;
    if (x_) *x_ = static_cast<EEL_F>(defaultX);
    if (y_) *y_ = static_cast<EEL_F>(defaultY);
    SampledColor baseColor = sampleColor(in, defaultX, defaultY);
    if (r_) *r_ = static_cast<EEL_F>(baseColor.r);
    if (g_) *g_ = static_cast<EEL_F>(baseColor.g);
    if (b_) *b_ = static_cast<EEL_F>(baseColor.b);

    if (pixelCode_) vm_.execute(pixelCode_);

    double xNorm = x_ ? static_cast<double>(*x_) : defaultX;
    double yNorm = y_ ? static_cast<double>(*y_) : defaultY;
    SampledColor fallbackColor = sampleColor(in, xNorm, yNorm);
    double red = r_ ? static_cast<double>(*r_) : fallbackColor.r;
    double green = g_ ? static_cast<double>(*g_) : fallbackColor.g;
    double blue = b_ ? static_cast<double>(*b_) : fallbackColor.b;
    std::array<std::uint8_t, 3> color{toByte(red), toByte(green), toByte(blue)};
    int px = toSuperscopeCoord(xNorm, w_);
    int py = toSuperscopeCoord(yNorm, h_);
    int thickness = 1;
    if (linesize_) {
      int lw = static_cast<int>(std::floor(*linesize_ + 0.5f));
      thickness = std::clamp(lw, 1, 255);
    }
    double skipValue = skip_ ? static_cast<double>(*skip_) : 0.0;
    bool lineMode = drawmode_ ? *drawmode_ > 0.00001f : false;
    if (skipValue <= 0.0) {
      if (lineMode && haveLast) {
        drawLine(out.rgba, w_, h_, lastX, lastY, px, py, color, thickness);
      } else {
        drawPoint(out.rgba, w_, h_, px, py, color, thickness);
      }
    }
    haveLast = true;
    lastX = px;
    lastY = py;
  }
}

}  // namespace avs
