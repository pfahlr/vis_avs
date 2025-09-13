#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "avs/audio.hpp"
#include "avs/eel.hpp"

namespace avs {

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

class BlurEffect : public Effect {
 public:
  explicit BlurEffect(int radius = 5);
  void init(int w, int h) override;
  void process(const Framebuffer& in, Framebuffer& out) override;
  void setRadius(int r);  ///< Update blur radius and kernel.
  int radius() const { return radius_; }

 private:
  int radius_;
  int w_ = 0;
  int h_ = 0;
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
  ScriptedEffect(std::string frameScript, std::string pixelScript);
  void init(int w, int h) override;
  void process(const Framebuffer& in, Framebuffer& out) override;
  void update(float time, int frame, const AudioState& audio);
  bool setScripts(std::string frameScript, std::string pixelScript);
  const std::string& lastError() const { return error_; }

 private:
  EelVm vm_;
  NSEEL_CODEHANDLE frameCode_ = nullptr;
  NSEEL_CODEHANDLE pixelCode_ = nullptr;
  std::string frameScript_;
  std::string pixelScript_;
  std::string error_;
  EEL_F *time_ = nullptr, *frame_ = nullptr, *bass_ = nullptr, *mid_ = nullptr, *treb_ = nullptr,
        *rms_ = nullptr;
  EEL_F *x_ = nullptr, *y_ = nullptr, *r_ = nullptr, *g_ = nullptr, *b_ = nullptr;
  int w_ = 0;
  int h_ = 0;
};

}  // namespace avs
