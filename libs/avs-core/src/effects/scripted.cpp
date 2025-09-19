#include <algorithm>
#include <cmath>

#include "avs/effects.hpp"

namespace avs {

ScriptedEffect::ScriptedEffect(std::string frameScript, std::string pixelScript) {
  setAllScripts("", std::move(frameScript), "", std::move(pixelScript));
}

ScriptedEffect::ScriptedEffect(std::string initScript,
                               std::string frameScript,
                               std::string beatScript,
                               std::string pixelScript) {
  setAllScripts(std::move(initScript),
                std::move(frameScript),
                std::move(beatScript),
                std::move(pixelScript));
}

ScriptedEffect::~ScriptedEffect() {
  if (initCode_) vm_.freeCode(initCode_);
  if (frameCode_) vm_.freeCode(frameCode_);
  if (beatCode_) vm_.freeCode(beatCode_);
  if (pixelCode_) vm_.freeCode(pixelCode_);
}

void ScriptedEffect::setScripts(std::string frameScript, std::string pixelScript) {
  setAllScripts("", std::move(frameScript), "", std::move(pixelScript));
}

void ScriptedEffect::setScripts(std::string initScript,
                                std::string frameScript,
                                std::string beatScript,
                                std::string pixelScript) {
  setAllScripts(std::move(initScript),
                std::move(frameScript),
                std::move(beatScript),
                std::move(pixelScript));
}

void ScriptedEffect::setAllScripts(std::string initScript,
                                   std::string frameScript,
                                   std::string beatScript,
                                   std::string pixelScript) {
  initScript_ = std::move(initScript);
  frameScript_ = std::move(frameScript);
  beatScript_ = std::move(beatScript);
  pixelScript_ = std::move(pixelScript);
  dirty_ = true;
  pendingBeat_ = false;
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
  beat_ = vm_.regVar("beat");
  x_ = vm_.regVar("x");
  y_ = vm_.regVar("y");
  r_ = vm_.regVar("red");
  g_ = vm_.regVar("green");
  b_ = vm_.regVar("blue");
  lastRms_ = 0.0f;
  initRan_ = false;
  pendingBeat_ = false;
  std::fill(legacyOsc_.begin(), legacyOsc_.end(), 0);
  std::fill(legacySpec_.begin(), legacySpec_.end(), 0);
  legacyChannels_ = 0;
  EelVm::LegacySources sources{};
  sources.oscBase = legacyOsc_.data();
  sources.specBase = legacySpec_.data();
  sources.sampleCount = AudioState::kLegacyVisSamples;
  sources.channels = legacyChannels_;
  vm_.setLegacySources(sources);
}

void ScriptedEffect::update(float time,
                            int frame,
                            const AudioState& audio,
                            const MouseState& mouse) {
  if (time_) *time_ = time;
  if (frame_) *frame_ = frame;
  if (bass_) *bass_ = audio.bands[0];
  if (mid_) *mid_ = audio.bands[1];
  if (treb_) *treb_ = audio.bands[2];
  if (rms_) *rms_ = audio.rms;
  if (beat_) {
    const float threshold = 0.6f;
    const bool isBeat = audio.rms > threshold && lastRms_ <= threshold;
    *beat_ = isBeat ? 1.0 : 0.0;
    pendingBeat_ = pendingBeat_ || isBeat;
    lastRms_ = audio.rms;
  }

  legacyChannels_ = std::clamp(audio.channels, 0, 2);
  const size_t sampleCount = AudioState::kLegacyVisSamples;
  for (int ch = 0; ch < 2; ++ch) {
    std::uint8_t* oscDst = legacyOsc_.data() + static_cast<size_t>(ch) * sampleCount;
    std::uint8_t* specDst = legacySpec_.data() + static_cast<size_t>(ch) * sampleCount;
    std::fill(oscDst, oscDst + sampleCount, 0);
    std::fill(specDst, specDst + sampleCount, 0);
    if (ch < legacyChannels_) {
      for (size_t i = 0; i < sampleCount; ++i) {
        float oscVal = audio.oscilloscope[ch][i];
        float specVal = audio.spectrumLegacy[ch][i];
        oscVal = std::clamp(oscVal, -1.0f, 1.0f);
        specVal = std::clamp(specVal, 0.0f, 1.0f);
        oscDst[i] = static_cast<std::uint8_t>(std::lround(oscVal * 127.5f + 127.5f));
        specDst[i] = static_cast<std::uint8_t>(std::lround(specVal * 255.0f));
      }
    }
  }
  if (legacyChannels_ <= 1) {
    std::copy_n(legacyOsc_.data(), sampleCount, legacyOsc_.data() + sampleCount);
    std::copy_n(legacySpec_.data(), sampleCount, legacySpec_.data() + sampleCount);
  }

  EelVm::LegacySources sources{};
  sources.oscBase = legacyOsc_.data();
  sources.specBase = legacySpec_.data();
  sources.sampleCount = sampleCount;
  sources.channels = legacyChannels_;
  sources.audioTimeSeconds = audio.timeSeconds;
  sources.engineTimeSeconds = time;
  sources.mouse = mouse;
  vm_.setLegacySources(sources);
}

void ScriptedEffect::compile() {
  if (!dirty_) return;
  if (initCode_) {
    vm_.freeCode(initCode_);
    initCode_ = nullptr;
  }
  if (frameCode_) {
    vm_.freeCode(frameCode_);
    frameCode_ = nullptr;
  }
  if (beatCode_) {
    vm_.freeCode(beatCode_);
    beatCode_ = nullptr;
  }
  if (pixelCode_) {
    vm_.freeCode(pixelCode_);
    pixelCode_ = nullptr;
  }
  if (!initScript_.empty()) initCode_ = vm_.compile(initScript_);
  if (!frameScript_.empty()) frameCode_ = vm_.compile(frameScript_);
  if (!beatScript_.empty()) beatCode_ = vm_.compile(beatScript_);
  if (!pixelScript_.empty()) pixelCode_ = vm_.compile(pixelScript_);
  dirty_ = false;
  initRan_ = false;
}

void ScriptedEffect::process(const Framebuffer& /*in*/, Framebuffer& out) {
  compile();

  if (!out.rgba.empty()) {
    std::fill(out.rgba.begin(), out.rgba.end(), 0);
  }
  if (!initRan_) {
    if (initCode_) vm_.execute(initCode_);
    initRan_ = true;
  }
  if (frameCode_) vm_.execute(frameCode_);
  if (pendingBeat_) {
    if (beatCode_) vm_.execute(beatCode_);
    pendingBeat_ = false;
  }
  for (int py = 0; py < h_; ++py) {
    if (y_) *y_ = py;
    for (int px = 0; px < w_; ++px) {
      if (x_) *x_ = px;
      if (pixelCode_) vm_.execute(pixelCode_);
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
