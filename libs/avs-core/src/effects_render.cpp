#include "avs/effects_render.hpp"
#include "avs/core.hpp"
#include "avs/params.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <random>

namespace avs {

// Utility to set a pixel in RGBA8 buffer safely.
static inline void put_px(FrameBufferView& fb,
                          int x,
                          int y,
                          const ColorRGBA8& c,
                          BlendMode mode = BlendMode::Replace) {
  if (x < 0 || y < 0 || x >= fb.width || y >= fb.height || !fb.data) return;
  uint8_t* p = fb.data + static_cast<size_t>(y) * fb.stride + static_cast<size_t>(x) * 4u;
  ColorRGBA8 dst{p[0], p[1], p[2], p[3]};
  ColorRGBA8 out = dst;
  blendPixel(out, c, mode);
  p[0] = out.r;
  p[1] = out.g;
  p[2] = out.b;
  p[3] = out.a;
}

static inline void line(FrameBufferView& fb, int x0, int y0, int x1, int y1, const ColorRGBA8& c){
  int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy, e2;
  for(;;){
    put_px(fb, x0, y0, c);
    if(x0 == x1 && y0 == y1) break;
    e2 = 2 * err;
    if(e2 >= dy){ err += dy; x0 += sx; }
    if(e2 <= dx){ err += dx; y0 += sy; }
  }
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

Param makeStringParam(std::string name, std::string value) {
  Param p;
  p.name = std::move(name);
  p.kind = ParamKind::String;
  p.value = std::move(value);
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

// ---------------- Oscilloscope ----------------
std::vector<Param> OscilloscopeEffect::parameters() const {
  std::vector<Param> params;
  params.push_back(makeSelectParam("source",
                                   "Mix",
                                   {OptionItem{"mix", "Mix"},
                                    OptionItem{"left", "Left"},
                                    OptionItem{"right", "Right"}}));
  params.push_back(makeSelectParam("draw_mode",
                                   "Lines",
                                   {OptionItem{"lines", "Lines"}, OptionItem{"dots", "Dots"}}));
  params.push_back(makeIntParam("thickness", 1, 1, 8));
  params.push_back(makeColorParam("color", ColorRGBA8{255, 255, 255, 255}));
  params.push_back(makeFloatParam("alpha", 1.0f, 0.0f, 1.0f));
  params.push_back(makeFloatParam("smoothing", 0.0f, 0.0f, 1.0f));
  return params;
}

void OscilloscopeEffect::process(const ProcessContext& ctx, FrameBufferView& dst){
  // Basic polyline oscilloscope using Mix channel.
  const auto& af = ctx.audio;
  const auto& wave = (af.oscL.size() >= af.oscR.size()) ? af.oscL : af.oscR;
  if(wave.empty()) return;
  ColorRGBA8 col{255,255,255,255};
  int W = dst.width, H = dst.height;
  int prevx = 0, prevy = H/2;
  for(int x=0; x<W; ++x){
    size_t idx = (size_t)((x / (float)W) * (wave.size()-1));
    float v = wave[idx]; // -1..1
    int y = (int)((0.5f - 0.5f * v) * (H-1));
    if(x > 0) line(dst, prevx, prevy, x, y, col);
    prevx = x; prevy = y;
  }
}

// ---------------- Spectrum Analyzer (stub) ----------------
std::vector<Param> SpectrumAnalyzerEffect::parameters() const {
  std::vector<Param> params;
  params.push_back(makeSelectParam("scale",
                                   "Linear",
                                   {OptionItem{"linear", "Linear"}, OptionItem{"log", "Log"}}));
  params.push_back(makeFloatParam("falloff", 0.2f, 0.0f, 1.0f));
  params.push_back(makeColorParam("color", ColorRGBA8{180, 220, 255, 255}));
  params.push_back(makeIntParam("bars", 64, 32, 512));
  params.push_back(makeFloatParam("alpha", 1.0f, 0.0f, 1.0f));
  return params;
}

void SpectrumAnalyzerEffect::process(const ProcessContext& ctx, FrameBufferView& dst){
  // Draw simple bars from spectrum magnitudes; if empty, generate from osc as fallback.
  int W = dst.width, H = dst.height;
  int bars = 64;
  std::vector<float> mags;
  if(!ctx.audio.spectrum.left.empty()){
    mags = ctx.audio.spectrum.left;
  }else if(!ctx.audio.oscL.empty()){
    // crude fallback
    mags.resize(bars);
    for(int i=0;i<bars;i++){
      float t = (float)i/(bars-1);
      mags[i] = 0.5f*(1.0f + std::sin((ctx.time.t_seconds*2 + t*10)*3.14159f));
    }
  }else{
    return;
  }
  int bw = std::max(1, W / (int)mags.size());
  for(size_t i=0;i<mags.size();++i){
    int h = (int)(std::clamp(mags[i], 0.0f, 1.0f) * (H-1));
    int x0 = (int)(i * bw);
    for(int y=H-1; y>=H-1-h; --y){
      for(int x=x0; x<std::min(W, x0 + bw - 1); ++x){
        put_px(dst, x, y, ColorRGBA8{180,220,255,255});
      }
    }
  }
}

// ---------------- Dots/Lines ----------------
std::vector<Param> DotsLinesEffect::parameters() const {
  std::vector<Param> params;
  params.push_back(makeIntParam("count", 512, 1, 4096));
  params.push_back(makeSelectParam("distribution",
                                   "Random",
                                   {OptionItem{"random", "Random"}, OptionItem{"grid", "Grid"}}));
  params.push_back(makeColorParam("color", ColorRGBA8{255, 255, 255, 255}));
  params.push_back(makeIntParam("thickness", 1, 1, 8));
  return params;
}

void DotsLinesEffect::process(const ProcessContext& ctx, FrameBufferView& dst){
  int W = dst.width, H = dst.height;
  int n = 512;
  // Deterministic seed on frame index for repeatability (demo only).
  std::minstd_rand rng(ctx.time.frame_index + 1337);
  std::uniform_int_distribution<int> dx(0, W-1), dy(0, H-1);
  for(int i=0;i<n;i++){
    int x = dx(rng), y = dy(rng);
    put_px(dst, x, y, ColorRGBA8{255,255,255,255});
  }
}

// ---------------- Starfield ----------------
struct Star { float x, y, z; };

std::vector<Param> StarfieldEffect::parameters() const {
  std::vector<Param> params;
  params.push_back(makeIntParam("stars", 1024, 64, 8192));
  params.push_back(makeFloatParam("speed", 1.0f, 0.0f, 5.0f));
  params.push_back(makeFloatParam("warp_center", 0.5f, 0.0f, 1.0f));
  params.push_back(makeColorParam("color", ColorRGBA8{255, 255, 255, 255}));
  return params;
}

class StarfieldState {
public:
  std::vector<Star> stars;
  int w{0}, h{0};
};

static StarfieldState& state(){
  static StarfieldState s; return s;
}

void StarfieldEffect::process(const ProcessContext& ctx, FrameBufferView& dst){
  (void)ctx;
  auto& S = state();
  if(S.w != dst.width || S.h != dst.height || S.stars.empty()){
    S.w = dst.width; S.h = dst.height;
    std::minstd_rand rng(42);
    std::uniform_real_distribution<float> U(-1.f, 1.f);
    S.stars.resize(1024);
    for(auto& s: S.stars){ s.x = U(rng); s.y = U(rng); s.z = (float)(0.2 + (std::fabs(U(rng))*1.0)); }
  }
  // Update and draw
  for(auto& s: S.stars){
    s.z -= 0.02f; if(s.z <= 0.01f){ s.z += 1.0f; }
    int x = (int)((s.x / s.z) * (S.w/4) + S.w/2);
    int y = (int)((s.y / s.z) * (S.h/4) + S.h/2);
    put_px(dst, x, y, ColorRGBA8{255,255,255,255});
  }
}

// ---------------- Text ----------------
namespace {

struct Glyph {
  std::array<uint8_t, 7> rows{};
};

Glyph glyphFor(char c) {
  switch (static_cast<unsigned char>(c)) {
    case 'A': return {{0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}};
    case 'V': return {{0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04}};
    case 'S': return {{0x1E, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E}};
    case '0': return {{0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}};
    case '1': return {{0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}};
    case '2': return {{0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}};
    case '3': return {{0x0E, 0x11, 0x01, 0x06, 0x01, 0x11, 0x0E}};
    case '4': return {{0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}};
    case '5': return {{0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E}};
    case '6': return {{0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E}};
    case '7': return {{0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}};
    case '8': return {{0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}};
    case '9': return {{0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C}};
    case ' ': return {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
    default: break;
  }
  return {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
}

void drawText(FrameBufferView& dst,
              int originX,
              int originY,
              const std::string& text,
              int scale,
              const ColorRGBA8& color) {
  const int advance = (5 * scale) + scale;  // 1px gap
  int xCursor = originX;
  for (char c : text) {
    Glyph glyph = glyphFor(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
    for (int row = 0; row < 7; ++row) {
      for (int col = 0; col < 5; ++col) {
        if ((glyph.rows[static_cast<size_t>(row)] & (1u << (4 - col))) == 0u) continue;
        for (int sy = 0; sy < scale; ++sy) {
          for (int sx = 0; sx < scale; ++sx) {
            put_px(dst,
                   xCursor + col * scale + sx,
                   originY + row * scale + sy,
                   color,
                   BlendMode::Replace);
          }
        }
      }
    }
    xCursor += advance;
  }
}

}  // namespace

std::vector<Param> TextEffect::parameters() const {
  std::vector<Param> params;
  params.push_back(makeStringParam("text", "AVS"));
  params.push_back(makeColorParam("color", ColorRGBA8{255, 255, 255, 255}));
  params.push_back(makeIntParam("scale", 2, 1, 6));
  return params;
}

void TextEffect::process(const ProcessContext&, FrameBufferView& dst) {
  const std::string text = "AVS";
  const int scale = 2;
  const ColorRGBA8 color{255, 255, 255, 255};
  if (dst.width <= 0 || dst.height <= 0) return;
  const int totalWidth = static_cast<int>(text.size()) * ((5 * scale) + scale) - scale;
  const int originX = std::max(0, (dst.width - totalWidth) / 2);
  const int originY = std::max(0, (dst.height - 7 * scale) / 2);
  drawText(dst, originX, originY, text, scale, color);
}

// ---------------- Picture ----------------
std::vector<Param> PictureEffect::parameters() const {
  std::vector<Param> params;
  params.push_back(makeSelectParam("mode",
                                   "Gradient",
                                   {OptionItem{"gradient", "Gradient"},
                                    OptionItem{"checker", "Checker"}}));
  params.push_back(makeColorParam("tint", ColorRGBA8{220, 200, 255, 255}));
  params.push_back(makeFloatParam("alpha", 1.0f, 0.0f, 1.0f));
  return params;
}

void PictureEffect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  if (!dst.data) return;
  const ColorRGBA8 tint{220, 200, 255, 255};
  const float phase = static_cast<float>(ctx.time.t_seconds);
  for (int y = 0; y < dst.height; ++y) {
    uint8_t* row = dst.data + static_cast<size_t>(y) * dst.stride;
    for (int x = 0; x < dst.width; ++x) {
      float nx = static_cast<float>(x) / std::max(1, dst.width - 1);
      float ny = static_cast<float>(y) / std::max(1, dst.height - 1);
      float radial = std::clamp(std::sqrt((nx - 0.5f) * (nx - 0.5f) + (ny - 0.5f) * (ny - 0.5f)), 0.0f, 1.0f);
      float osc = 0.5f + 0.5f * std::sin(phase + radial * 6.28318f);
      ColorRGBA8 c{
          static_cast<uint8_t>(std::clamp(osc * tint.r, 0.0f, 255.0f)),
          static_cast<uint8_t>(std::clamp((1.0f - radial) * 255.0f, 0.0f, 255.0f)),
          static_cast<uint8_t>(std::clamp(osc * tint.b, 0.0f, 255.0f)),
          tint.a,
      };
      row[x * 4 + 0] = c.r;
      row[x * 4 + 1] = c.g;
      row[x * 4 + 2] = c.b;
      row[x * 4 + 3] = c.a;
    }
  }
}

// ---------------- Superscope ----------------
void SuperscopeEffect::init(const InitContext&) {}

std::vector<Param> SuperscopeEffect::parameters() const {
  std::vector<Param> params;
  params.push_back(makeIntParam("points", 512, 32, 4096));
  params.push_back(makeIntParam("thickness", 1, 1, 8));
  params.push_back(makeColorParam("color", ColorRGBA8{64, 255, 192, 255}));
  return params;
}

void SuperscopeEffect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  const int points = 512;
  const ColorRGBA8 color{64, 255, 192, 255};
  if (dst.width <= 0 || dst.height <= 0) return;

  const auto& osc = !ctx.audio.oscL.empty() ? ctx.audio.oscL : ctx.audio.oscR;
  auto sampleWave = [&](float t) {
    if (osc.empty()) return std::sin(t);
    float pos = std::clamp(t, 0.0f, 1.0f) * static_cast<float>(osc.size() - 1);
    int base = static_cast<int>(pos);
    float frac = pos - static_cast<float>(base);
    float a = osc[static_cast<size_t>(base)];
    float b = osc[std::min(static_cast<size_t>(base + 1), osc.size() - 1)];
    return a + (b - a) * frac;
  };

  int prevX = -1;
  int prevY = -1;
  for (int i = 0; i < points; ++i) {
    float t = static_cast<float>(i) / std::max(1, points - 1);
    float angle = ctx.time.t_seconds * 0.6f + t * 6.28318f;
    float radius = 0.35f + 0.15f * sampleWave(t);
    float px = 0.5f + std::cos(angle) * radius;
    float py = 0.5f + std::sin(angle) * radius * sampleWave(1.0f - t);
    int x = static_cast<int>(px * static_cast<float>(dst.width - 1));
    int y = static_cast<int>(py * static_cast<float>(dst.height - 1));
    if (prevX >= 0) {
      line(dst, prevX, prevY, x, y, color);
    }
    prevX = x;
    prevY = y;
  }
}

// ---------------- Triangles ----------------
std::vector<Param> TrianglesEffect::parameters() const {
  std::vector<Param> params;
  params.push_back(makeIntParam("count", 3, 1, 8));
  params.push_back(makeColorParam("color", ColorRGBA8{255, 200, 120, 255}));
  return params;
}

void TrianglesEffect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  const ColorRGBA8 color{255, 200, 120, 255};
  const int count = 3;
  if (dst.width <= 0 || dst.height <= 0) return;

  const float baseAngle = static_cast<float>(ctx.time.t_seconds) * 0.7f;
  const int cx = dst.width / 2;
  const int cy = dst.height / 2;
  const float radius = std::min(dst.width, dst.height) * 0.35f;

  for (int i = 0; i < count; ++i) {
    float angle = baseAngle + static_cast<float>(i) * 2.09439f;  // 120 degrees apart
    int x0 = cx + static_cast<int>(std::cos(angle) * radius);
    int y0 = cy + static_cast<int>(std::sin(angle) * radius);
    int x1 = cx + static_cast<int>(std::cos(angle + 2.09439f) * radius);
    int y1 = cy + static_cast<int>(std::sin(angle + 2.09439f) * radius);
    int x2 = cx + static_cast<int>(std::cos(angle + 4.18878f) * radius);
    int y2 = cy + static_cast<int>(std::sin(angle + 4.18878f) * radius);
    line(dst, x0, y0, x1, y1, color);
    line(dst, x1, y1, x2, y2, color);
    line(dst, x2, y2, x0, y0, color);
  }
}

// ---------------- Shapes ----------------
std::vector<Param> ShapesEffect::parameters() const {
  std::vector<Param> params;
  params.push_back(makeSelectParam("mode",
                                   "Circle",
                                   {OptionItem{"circle", "Circle"},
                                    OptionItem{"rect", "Rectangle"}}));
  params.push_back(makeColorParam("color", ColorRGBA8{180, 240, 255, 255}));
  return params;
}

void ShapesEffect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  if (dst.width <= 0 || dst.height <= 0) return;
  const ColorRGBA8 color{180, 240, 255, 255};
  const int cx = dst.width / 2;
  const int cy = dst.height / 2;
  const int radius = std::max(2, std::min(dst.width, dst.height) / 4);
  const bool drawCircle = (ctx.time.frame_index % 2) == 0;

  if (drawCircle) {
    for (int y = -radius; y <= radius; ++y) {
      for (int x = -radius; x <= radius; ++x) {
        if (x * x + y * y > radius * radius) continue;
        put_px(dst, cx + x, cy + y, color, BlendMode::Replace);
      }
    }
  } else {
    for (int y = -radius; y <= radius; ++y) {
      for (int x = -radius; x <= radius; ++x) {
        if (std::abs(x) == radius || std::abs(y) == radius) {
          put_px(dst, cx + x, cy + y, color, BlendMode::Replace);
        }
      }
    }
  }
}

// ---------------- Dot Grid ----------------
std::vector<Param> DotGridEffect::parameters() const {
  std::vector<Param> params;
  params.push_back(makeIntParam("spacing", 16, 4, 64));
  params.push_back(makeColorParam("color", ColorRGBA8{255, 255, 255, 255}));
  return params;
}

void DotGridEffect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  const int spacing = 16;
  const ColorRGBA8 color{255, 255, 255, 200};
  if (dst.width <= 0 || dst.height <= 0) return;

  const float jitter = 0.5f * std::sin(static_cast<float>(ctx.time.t_seconds) * 1.7f);
  for (int y = 0; y < dst.height; y += spacing) {
    for (int x = 0; x < dst.width; x += spacing) {
      int px = static_cast<int>(x + jitter * spacing);
      int py = static_cast<int>(y + jitter * spacing);
      put_px(dst, px, py, color, BlendMode::Add);
    }
  }
}

} // namespace avs
