#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "avs/audio.hpp"
#include "avs/eel.hpp"

namespace avs {

static_assert(EelVm::kLegacyVisSamples == AudioState::kLegacyVisSamples,
              "Legacy vis sample count mismatch");

struct Framebuffer {
  int w = 0;
  int h = 0;
  std::vector<std::uint8_t> rgba;
};

class Effect {
 public:
  virtual ~Effect() = default;
  virtual void init(int w, int h) {
    (void)w;
    (void)h;
  }
  virtual void process(const Framebuffer& in, Framebuffer& out) = 0;
};

class CompositeEffect : public Effect {
 public:
  void addEffect(std::unique_ptr<Effect> effect);
  void init(int w, int h) override;
  void process(const Framebuffer& in, Framebuffer& out) override;

  size_t childCount() const { return children_.size(); }
  const std::vector<std::unique_ptr<Effect>>& children() const { return children_; }

 private:
  std::vector<std::unique_ptr<Effect>> children_;
  Framebuffer buffers_[2];
  int width_ = 0;
  int height_ = 0;
};

class BlurEffect : public Effect {
 public:
  explicit BlurEffect(int radius = 5);
  void init(int w, int h) override;
  void process(const Framebuffer& in, Framebuffer& out) override;

 private:
  int radius_;
  std::vector<float> kernel_;
  Framebuffer temp_;
};

class ColorMapEffect : public Effect {
 public:
  void init(int w, int h) override;
  void process(const Framebuffer& in, Framebuffer& out) override;

 private:
  std::array<std::uint8_t, 256 * 3> lut_;
};

class ConvolutionEffect : public Effect {
 public:
  void init(int w, int h) override;
  void process(const Framebuffer& in, Framebuffer& out) override;

 private:
  std::array<int, 9> kernel_;
};

class MotionBlurEffect : public Effect {
 public:
  void init(int w, int h) override;
  void process(const Framebuffer& in, Framebuffer& out) override;

 private:
  Framebuffer prev_;
};

class FadeoutEffect : public Effect {
 public:
  FadeoutEffect(int fadeLen, std::uint32_t color);
  void init(int w, int h) override;
  void process(const Framebuffer& in, Framebuffer& out) override;

 private:
  void recomputeLuts();
  int fadeLen_ = 0;
  std::uint8_t targetR_ = 0;
  std::uint8_t targetG_ = 0;
  std::uint8_t targetB_ = 0;
  std::array<std::uint8_t, 256> lutR_{};
  std::array<std::uint8_t, 256> lutG_{};
  std::array<std::uint8_t, 256> lutB_{};
};

class ColorTransformEffect : public Effect {
 public:
  void init(int w, int h) override;
  void process(const Framebuffer& in, Framebuffer& out) override;
};

class GlowEffect : public Effect {
 public:
  void process(const Framebuffer& in, Framebuffer& out) override;
};

class ZoomRotateEffect : public Effect {
 public:
  void init(int w, int h) override;
  void process(const Framebuffer& in, Framebuffer& out) override;
};

class MirrorEffect : public Effect {
 public:
  void init(int w, int h) override;
  void process(const Framebuffer& in, Framebuffer& out) override;
};

class TunnelEffect : public Effect {
 public:
  void init(int w, int h) override;
  void process(const Framebuffer& in, Framebuffer& out) override;

 private:
  int cx_ = 0;
  int cy_ = 0;
};

class RadialBlurEffect : public Effect {
 public:
  void init(int w, int h) override;
  void process(const Framebuffer& in, Framebuffer& out) override;
};

class AdditiveBlendEffect : public Effect {
 public:
  void init(int w, int h) override;
  void process(const Framebuffer& in, Framebuffer& out) override;

 private:
  Framebuffer blend_;
};

class ScriptedEffect : public Effect {
 public:
  enum class Mode { kSuperscope, kColorModifier };

  ScriptedEffect(std::string frameScript,
                 std::string pixelScript,
                 Mode mode = Mode::kSuperscope,
                 bool colorModRecompute = false);
  ScriptedEffect(std::string initScript,
                 std::string frameScript,
                 std::string beatScript,
                 std::string pixelScript,
                 Mode mode = Mode::kSuperscope,
                 bool colorModRecompute = false);
  ~ScriptedEffect() override;
  void init(int w, int h) override;
  void process(const Framebuffer& in, Framebuffer& out) override;
  void update(float time, int frame, const AudioState& audio, const MouseState& mouse);
  void setScripts(std::string frameScript, std::string pixelScript);
  void setScripts(std::string initScript,
                  std::string frameScript,
                  std::string beatScript,
                  std::string pixelScript);
  const std::string& initScript() const { return initScript_; }
  const std::string& frameScript() const { return frameScript_; }
  const std::string& beatScript() const { return beatScript_; }
  const std::string& pixelScript() const { return pixelScript_; }

 private:
  void setAllScripts(std::string initScript,
                     std::string frameScript,
                     std::string beatScript,
                     std::string pixelScript);
  void compile();
  EelVm vm_;
  NSEEL_CODEHANDLE initCode_ = nullptr;
  NSEEL_CODEHANDLE frameCode_ = nullptr;
  NSEEL_CODEHANDLE beatCode_ = nullptr;
  NSEEL_CODEHANDLE pixelCode_ = nullptr;
  std::string initScript_;
  std::string frameScript_;
  std::string beatScript_;
  std::string pixelScript_;
  bool dirty_ = true;
  bool initRan_ = false;
  bool pendingBeat_ = false;
  bool isBeatFrame_ = false;
  float lastRms_ = 0.0f;
  Mode mode_ = Mode::kSuperscope;
  bool colorModRecompute_ = false;
  bool colorLutDirty_ = true;
  std::array<std::uint8_t, 256 * 3> colorLut_{};
  EEL_F *time_ = nullptr, *frame_ = nullptr, *bass_ = nullptr, *mid_ = nullptr, *treb_ = nullptr,
        *rms_ = nullptr, *beat_ = nullptr;
  EEL_F *bVar_ = nullptr, *n_ = nullptr, *i_ = nullptr, *v_ = nullptr, *wVar_ = nullptr,
        *hVar_ = nullptr, *skip_ = nullptr, *linesize_ = nullptr, *drawmode_ = nullptr;
  EEL_F *x_ = nullptr, *y_ = nullptr, *r_ = nullptr, *g_ = nullptr, *b_ = nullptr;
  int w_ = 0;
  int h_ = 0;
  std::array<std::uint8_t, EelVm::kLegacyVisSamples * 2> legacyOsc_{};
  std::array<std::uint8_t, EelVm::kLegacyVisSamples * 2> legacySpec_{};
  int legacyChannels_ = 0;
  std::array<float, 576> waveform_{};
};

}  // namespace avs
