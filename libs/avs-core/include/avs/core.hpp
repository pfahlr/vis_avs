#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <variant>
#include <functional>
#include <optional>
#include <span>
#include <array>

// AVS core common types and enums.
// Indentation: 2 spaces.

namespace avs {

// ---------- Math / Utility ----------

struct Vec2i {
  int x{0};
  int y{0};
};

struct Vec2f {
  float x{0.f};
  float y{0.f};
};

struct ColorRGBA8 {
  uint8_t r{0}, g{0}, b{0}, a{255};
};

enum class BlendMode : uint8_t {
  Replace,
  Add,
  Subtract,
  Multiply,
  Max,
  Min,
  Average,
  Xor,
  Fifty // 50/50 crossfade
};

enum class Filter : uint8_t { Nearest, Bilinear };
enum class Wrap : uint8_t { Clamp, Wrap, Mirror };

enum class EffectGroup : uint8_t { Render, Trans, Misc };

// ---------- Audio / Analysis ----------

struct AudioStreamSpec {
  int sample_rate{44100};
  int channels{2};
  int block_size{1024}; // frames per analysis hop
};

struct AudioBlock {
  // Interleaved float32 samples, size = frames * channels
  std::span<const float> interleaved;
  int frames{0};
  AudioStreamSpec spec{};
};

struct Spectrum {
  // Magnitudes for L/R. If mono, only L is used.
  std::vector<float> left;
  std::vector<float> right;
  bool log_scale{false};
};

struct AudioFeatures {
  // Time-domain oscilloscope samples for visualization (normalized -1..1).
  std::vector<float> oscL;
  std::vector<float> oscR;
  Spectrum spectrum; // linear or log mapping
  bool beat{false};
  float bass{0.f}, mid{0.f}, treb{0.f};
  int sample_rate{44100};
};

// ---------- Framebuffers ----------

struct FrameSize {
  int w{640};
  int h{480};
};

// Simple RGBA8 framebuffer view
struct FrameBufferView {
  uint8_t* data{nullptr};
  int width{0};
  int height{0};
  int stride{0}; // bytes per row
};

struct FrameBuffers {
  FrameBufferView current;
  FrameBufferView previous; // last frame
  // Optional named registers A..E may be managed by the engine; effects access via API.
};

// ---------- Randomness / Determinism ----------
struct RNG {
  // Placeholder; implement xoshiro128** in .cpp and expose via interface if needed.
  uint64_t s[2]{0, 0};
};

// ---------- Forward Decls ----------
struct EngineCaps;
struct TimingInfo;
class EELContext; // Forward-declare a scripting environment wrapper.

struct TimingInfo {
  double t_seconds{0.0}; // total time since load
  int frame_index{0};
  double dt_seconds{1.0 / 60.0};
  bool deterministic{false};
  int fps_hint{60};
};

struct EngineCaps {
  bool has_sdl{true};
  bool has_gl{false};
  bool has_avx2{false};
  bool has_eel2{true};
};

// ---------- Utility helpers (declarations only) ----------
// Pixel sampling helper contract for CPU path (implement in .cpp).
struct SampleOptions {
  Filter filter{Filter::Nearest};
  Wrap wrap{Wrap::Clamp};
};

ColorRGBA8 sampleRGBA(const FrameBufferView& src, float x, float y, const SampleOptions& opt);
void blendPixel(ColorRGBA8& dst, const ColorRGBA8& src, BlendMode mode);

} // namespace avs
