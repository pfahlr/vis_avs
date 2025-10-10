#pragma once
#include <memory>
#include <vector>
#include <string>
#include <string_view>
#include <functional>
#include "core.hpp"
#include "params.hpp"

namespace avs {

// Contexts provided to effects.
struct InitContext {
  FrameSize frame_size;
  EngineCaps caps;
  bool deterministic{false};
  int fps_hint{60};
};

struct ProcessContext {
  const TimingInfo& time;
  const AudioFeatures& audio;
  const FrameBuffers& fb;
  RNG* rng{nullptr};           // optional deterministic RNG
  EELContext* eel{nullptr};    // optional EEL2 environment (shared)
};

class IEffect {
public:
  virtual ~IEffect() = default;
  virtual EffectGroup group() const = 0;
  virtual std::string_view name() const = 0;

  // Called once when inserted/compiled.
  virtual void init(const InitContext& ctx) { (void)ctx; }

  // For Render/Misc that paint into current or manipulate engine state.
  // 'dst' points to the engine's current framebuffer for this pass.
  virtual void process(const ProcessContext& ctx, FrameBufferView& dst) = 0;

  // Parameter reflection (optional but helpful for editors/serialization).
  virtual std::vector<Param> parameters() const { return {}; }
  virtual void set_parameter(std::string_view, const ParamValue&) {}
};

using EffectFactory = std::function<std::unique_ptr<IEffect>()>;

struct EffectDescriptor {
  std::string id;           // stable identifier (e.g., "superscope")
  std::string label;        // human-readable
  EffectGroup group{EffectGroup::Render};
  EffectFactory factory;
};

} // namespace avs
