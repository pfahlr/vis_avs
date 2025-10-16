#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "avs/core.hpp"
#include "avs/eel.hpp"
#include "avs/effect.hpp"

namespace avs::effects::geometry {

struct SuperscopeConfig {
  std::string initScript;
  std::string frameScript;
  std::string beatScript;
  std::string pointScript;
};

class SuperscopeRuntime {
 public:
  SuperscopeRuntime();
  ~SuperscopeRuntime();

  void setScripts(const SuperscopeConfig &config);
  void init(const InitContext &ctx);
  void update(const ProcessContext &ctx);
  void render(const ProcessContext &ctx, FrameBufferView &dst);
  void setOverrides(std::optional<int> points, std::optional<float> thickness,
                    std::optional<bool> lineMode);

 private:
  void compile();
  void ensureBuffers();

  EelVm vm_;
  SuperscopeConfig config_{};
  bool dirty_{true};
  bool initRan_{false};
  bool pendingBeat_{false};

  NSEEL_CODEHANDLE initCode_{nullptr};
  NSEEL_CODEHANDLE frameCode_{nullptr};
  NSEEL_CODEHANDLE beatCode_{nullptr};
  NSEEL_CODEHANDLE pointCode_{nullptr};

  EEL_F *time_{nullptr};
  EEL_F *frame_{nullptr};
  EEL_F *bass_{nullptr};
  EEL_F *mid_{nullptr};
  EEL_F *treb_{nullptr};
  EEL_F *rms_{nullptr};
  EEL_F *beat_{nullptr};
  EEL_F *bVar_{nullptr};
  EEL_F *n_{nullptr};
  EEL_F *i_{nullptr};
  EEL_F *v_{nullptr};
  EEL_F *wVar_{nullptr};
  EEL_F *hVar_{nullptr};
  EEL_F *skip_{nullptr};
  EEL_F *lineSize_{nullptr};
  EEL_F *drawMode_{nullptr};
  EEL_F *x_{nullptr};
  EEL_F *y_{nullptr};
  EEL_F *r_{nullptr};
  EEL_F *g_{nullptr};
  EEL_F *b_{nullptr};

  int width_{0};
  int height_{0};
  float lastRms_{0.0f};

  std::array<float, 576> waveform_{};
  std::array<std::uint8_t, EelVm::kLegacyVisSamples * 2> legacyOsc_{};
  std::array<std::uint8_t, EelVm::kLegacyVisSamples * 2> legacySpec_{};
  int legacyChannels_{0};

  std::optional<int> overridePoints_;
  std::optional<float> overrideThickness_;
  std::optional<bool> overrideLineMode_;
};

}  // namespace avs::effects::geometry
