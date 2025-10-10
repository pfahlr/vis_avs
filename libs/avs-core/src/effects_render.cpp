#include "avs/effects_render.hpp"
#include "avs/core.hpp"
#include "avs/params.hpp"
#include <cmath>
#include <random>
#include <algorithm>

namespace avs {

// Utility to set a pixel in RGBA8 buffer safely.
static inline void put_px(FrameBufferView& fb, int x, int y, const ColorRGBA8& c){
  if(x < 0 || y < 0 || x >= fb.width || y >= fb.height) return;
  uint8_t* p = fb.data + y * fb.stride + x * 4;
  p[0] = c.r; p[1] = c.g; p[2] = c.b; p[3] = c.a;
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

// ---------------- Oscilloscope ----------------
std::vector<Param> OscilloscopeEffect::parameters() const {
  return {
    {"source", ParamKind::Select, std::string("Mix")},
    {"draw_mode", ParamKind::Select, std::string("Lines")},
    {"thickness", ParamKind::Int, 1, 1, 8},
    {"color", ParamKind::Color, ColorRGBA8{255,255,255,255}},
    {"alpha", ParamKind::Float, 1.0f, {}, {}, {} ,},
    {"smoothing", ParamKind::Float, 0.0f, 0.0f, 1.0f}
  };
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
  return {
    {"scale", ParamKind::Select, std::string("Linear")},
    {"falloff", ParamKind::Float, 0.2f, 0.0f, 1.0f},
    {"color", ParamKind::Color, ColorRGBA8{180,220,255,255}},
    {"bars", ParamKind::Int, 64, 32, 512},
    {"alpha", ParamKind::Float, 1.0f, 0.0f, 1.0f}
  };
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
  return {
    {"count", ParamKind::Int, 512, 1, 4096},
    {"distribution", ParamKind::Select, std::string("Random")},
    {"color", ParamKind::Color, ColorRGBA8{255,255,255,255}},
    {"thickness", ParamKind::Int, 1, 1, 8}
  };
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
  return {
    {"stars", ParamKind::Int, 1024, 64, 8192},
    {"speed", ParamKind::Float, 1.0f, 0.0f, 5.0f},
    {"warp_center", ParamKind::Float, 0.5f, 0.0f, 1.0f},
    {"color", ParamKind::Color, ColorRGBA8{255,255,255,255}},
  };
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

// ---------------- Text / Picture / Superscope / Triangles / Shapes / DotGrid ----------------
// Stubs for now.
std::vector<Param> TextEffect::parameters() const { return {}; }
void TextEffect::process(const ProcessContext&, FrameBufferView&) {}

std::vector<Param> PictureEffect::parameters() const { return {}; }
void PictureEffect::process(const ProcessContext&, FrameBufferView&) {}

void SuperscopeEffect::init(const InitContext&) {}
std::vector<Param> SuperscopeEffect::parameters() const { return {}; }
void SuperscopeEffect::process(const ProcessContext&, FrameBufferView&) {}

std::vector<Param> TrianglesEffect::parameters() const { return {}; }
void TrianglesEffect::process(const ProcessContext&, FrameBufferView&) {}

std::vector<Param> ShapesEffect::parameters() const { return {}; }
void ShapesEffect::process(const ProcessContext&, FrameBufferView&) {}

std::vector<Param> DotGridEffect::parameters() const { return {}; }
void DotGridEffect::process(const ProcessContext&, FrameBufferView&) {}

} // namespace avs
