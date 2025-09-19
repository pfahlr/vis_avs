#include "avs/engine.hpp"

#include <algorithm>
#include <cmath>

#include "avs/audio.hpp"
#include "avs/effects.hpp"

namespace avs {

Engine::Engine(int w, int h) { alloc(w, h); }

void Engine::alloc(int w, int h) {
  w_ = w;
  h_ = h;
  for (auto& fb : fb_) {
    fb.w = w;
    fb.h = h;
    fb.rgba.resize(static_cast<size_t>(w) * h * 4);
  }
}

void Engine::resize(int w, int h) {
  if (w == w_ && h == h_) return;
  alloc(w, h);
  for (auto& e : chain_) {
    e->init(w_, h_);
  }
}

void Engine::setAudio(const AudioState& a) { audio_ = a; }

void Engine::setMouseState(const MouseState& mouse) { mouse_ = mouse; }

void Engine::setChain(std::vector<std::unique_ptr<Effect>> chain) {
  chain_ = std::move(chain);
  for (auto& e : chain_) {
    e->init(w_, h_);
  }
}

void Engine::step(float dt) {
  time_ += dt;
  ++frame_;
  auto& base = fb_[0];
  for (int y = 0; y < h_; ++y) {
    for (int x = 0; x < w_; ++x) {
      size_t idx = (static_cast<size_t>(y) * w_ + x) * 4;
      base.rgba[idx + 0] = static_cast<std::uint8_t>(x + time_ * 100);
      base.rgba[idx + 1] = static_cast<std::uint8_t>(y + time_ * 100);
      base.rgba[idx + 2] = static_cast<std::uint8_t>((x + y) + time_ * 100);
      base.rgba[idx + 3] = 255;
    }
  }
  int in = 0;
  int out = 1;
  for (auto& e : chain_) {
    if (auto* se = dynamic_cast<ScriptedEffect*>(e.get())) {
      se->update(time_, frame_, audio_, mouse_);
    }
    e->process(fb_[in], fb_[out]);
    std::swap(in, out);
  }
  cur_ = in;
}

const Framebuffer& Engine::frame() const { return fb_[cur_]; }

}  // namespace avs
