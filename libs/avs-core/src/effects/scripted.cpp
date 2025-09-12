#include <algorithm>

#include "avs/effects.hpp"

namespace avs {

ScriptedEffect::ScriptedEffect(std::string frameScript, std::string pixelScript)
    : frameScript_(std::move(frameScript)), pixelScript_(std::move(pixelScript)) {}

void ScriptedEffect::setScripts(std::string frameScript, std::string pixelScript) {
  frameScript_ = std::move(frameScript);
  pixelScript_ = std::move(pixelScript);
  dirty_ = true;
}

void ScriptedEffect::init(int w, int h) {
  w_ = w;
  h_ = h;
  time_ = vm_.regVar("time");
  frame_ = vm_.regVar("frame");
  bass_ = vm_.regVar("bass");
  mid_ = vm_.regVar("mid");
  treb_ = vm_.regVar("treb");
  rms_ = vm_.regVar("rms");
  x_ = vm_.regVar("x");
  y_ = vm_.regVar("y");
  r_ = vm_.regVar("red");
  g_ = vm_.regVar("green");
  b_ = vm_.regVar("blue");
}

void ScriptedEffect::update(float time, int frame, const AudioState& audio) {
  if (time_) *time_ = time;
  if (frame_) *frame_ = frame;
  if (bass_) *bass_ = audio.bands[0];
  if (mid_) *mid_ = audio.bands[1];
  if (treb_) *treb_ = audio.bands[2];
  if (rms_) *rms_ = audio.rms;
}

void ScriptedEffect::process(const Framebuffer& /*in*/, Framebuffer& out) {
  if (dirty_) {
    if (frameCode_) vm_.freeCode(frameCode_);
    if (pixelCode_) vm_.freeCode(pixelCode_);
    frameCode_ = vm_.compile(frameScript_);
    pixelCode_ = vm_.compile(pixelScript_);
    dirty_ = false;
  }

  vm_.execute(frameCode_);
  for (int py = 0; py < h_; ++py) {
    if (y_) *y_ = py;
    for (int px = 0; px < w_; ++px) {
      if (x_) *x_ = px;
      vm_.execute(pixelCode_);
      int idx = (py * w_ + px) * 4;
      auto toByte = [](double v) {
        return static_cast<std::uint8_t>(std::clamp(v, 0.0, 1.0) * 255.0 + 0.5);
      };
      out.rgba[idx + 0] = toByte(r_ ? *r_ : 0.0);
      out.rgba[idx + 1] = toByte(g_ ? *g_ : 0.0);
      out.rgba[idx + 2] = toByte(b_ ? *b_ : 0.0);
      out.rgba[idx + 3] = 255;
    }
  }
}

}  // namespace avs
