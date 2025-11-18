#include <avs/engine.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>

#include <avs/audio.hpp>
#include <avs/effects.hpp>

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

  // Start with the current framebuffer (previous frame)
  // Effects will render on top of this
  int in = cur_;
  int out = 1 - cur_;

  // Debug: Check buffer contents before effects
  if (frame_ <= 3) {
    int nonzero = 0;
    for (size_t i = 0; i < std::min(size_t(100), fb_[in].rgba.size()); ++i) {
      if (fb_[in].rgba[i] != 0) nonzero++;
    }
    std::fprintf(stderr, "Frame %d BEFORE: cur_=%d in=%d out=%d, first 100 bytes nonzero=%d, first pixels: %d %d %d %d\n",
                 frame_, cur_, in, out, nonzero,
                 fb_[in].rgba.size() > 0 ? fb_[in].rgba[0] : -1,
                 fb_[in].rgba.size() > 1 ? fb_[in].rgba[1] : -1,
                 fb_[in].rgba.size() > 2 ? fb_[in].rgba[2] : -1,
                 fb_[in].rgba.size() > 3 ? fb_[in].rgba[3] : -1);
  }

  for (auto& e : chain_) {
    if (auto* se = dynamic_cast<ScriptedEffect*>(e.get())) {
      se->update(time_, frame_, audio_, mouse_);
    }
    e->process(fb_[in], fb_[out]);
    std::swap(in, out);
  }
  cur_ = in;

  // Debug: Check buffer contents after effects
  if (frame_ <= 3) {
    int nonzero = 0;
    for (size_t i = 0; i < std::min(size_t(100), fb_[cur_].rgba.size()); ++i) {
      if (fb_[cur_].rgba[i] != 0) nonzero++;
    }
    std::fprintf(stderr, "Frame %d AFTER: cur_=%d, first 100 bytes nonzero=%d, first pixels: %d %d %d %d\n",
                 frame_, cur_, nonzero,
                 fb_[cur_].rgba.size() > 0 ? fb_[cur_].rgba[0] : -1,
                 fb_[cur_].rgba.size() > 1 ? fb_[cur_].rgba[1] : -1,
                 fb_[cur_].rgba.size() > 2 ? fb_[cur_].rgba[2] : -1,
                 fb_[cur_].rgba.size() > 3 ? fb_[cur_].rgba[3] : -1);
  }
}

const Framebuffer& Engine::frame() const { return fb_[cur_]; }

}  // namespace avs
