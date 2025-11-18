#include <avs/engine.hpp>

#include <algorithm>
#include <cmath>

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

  // Debug: Print every 60 frames to verify step is being called
  if (frame_ % 60 == 0) {
    std::fprintf(stderr, "Engine::step frame %d, time %.2f, dt %.4f\n", frame_, time_, dt);
  }

  // Start with the current framebuffer (previous frame)
  // Effects will render on top of this
  int in = cur_;
  int out = 1 - cur_;

  // Copy the previous frame to the output buffer before processing effects.
  // This enables frame-to-frame persistence, allowing effects to build upon
  // the previous frame's content (critical for BufferSave/Restore and other
  // temporal effects).
  if (fb_[out].rgba.size() != fb_[in].rgba.size()) {
    fb_[out].rgba.resize(fb_[in].rgba.size());
  }
  std::copy(fb_[in].rgba.begin(), fb_[in].rgba.end(), fb_[out].rgba.begin());

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
