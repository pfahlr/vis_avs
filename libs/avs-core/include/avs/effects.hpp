#pragma once

#include <array>
#include <cstdint>
#include <vector>

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

}  // namespace avs
