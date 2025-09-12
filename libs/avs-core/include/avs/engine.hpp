#pragma once

#include <array>
#include <memory>
#include <vector>

#include "avs/audio.hpp"
#include "avs/effects.hpp"

namespace avs {

class Engine {
 public:
  Engine(int w, int h);

  void resize(int w, int h);
  void setAudio(const AudioState& a);
  void step(float dt);
  const Framebuffer& frame() const;
  void setChain(std::vector<std::unique_ptr<Effect>> chain);

 private:
  void alloc(int w, int h);

  std::array<Framebuffer, 2> fb_{};
  int w_ = 0;
  int h_ = 0;
  int cur_ = 0;
  std::vector<std::unique_ptr<Effect>> chain_;
  AudioState audio_{};
  float time_ = 0.0f;
};

}  // namespace avs
