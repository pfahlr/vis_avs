#include "avs/script/EELContext.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace avs {

#if AVS_ENABLE_EEL2

EELContext::EELContext() : vm_(std::make_unique<EelVm>()) {
  oscBuffer_.resize(EelVm::kLegacyVisSamples * 2u, 0);
  specBuffer_.resize(EelVm::kLegacyVisSamples * 2u, 0);
}

EELContext::~EELContext() = default;

bool EELContext::isEnabled() const { return true; }

void EELContext::setVariable(std::string_view name, double value) {
  std::string key(name);
  auto it = variables_.find(key);
  if (it == variables_.end()) {
    EEL_F* slot = vm_->regVar(key.c_str());
    if (!slot) return;
    it = variables_.emplace(key, slot).first;
  }
  *(it->second) = static_cast<EEL_F>(value);
}

double EELContext::getVariable(std::string_view name) const {
  auto it = variables_.find(std::string(name));
  return it != variables_.end() ? static_cast<double>(*(it->second)) : 0.0;
}

bool EELContext::compile(std::string_view name, std::string_view code) {
  if (!vm_) return false;
  std::string key(name);
  Program prog{};
  prog.name = key;
  prog.code = std::string(code);
  prog.handle = vm_->compile(prog.code);
  if (!prog.handle) {
    return false;
  }
  auto [it, inserted] = programs_.emplace(key, prog);
  if (!inserted) {
    if (it->second.handle) vm_->freeCode(it->second.handle);
    it->second = prog;
  }
  return true;
}

bool EELContext::execute(std::string_view name) {
  auto it = programs_.find(std::string(name));
  if (it == programs_.end() || !it->second.handle) return false;
  vm_->execute(it->second.handle);
  return true;
}

void EELContext::remove(std::string_view name) {
  auto it = programs_.find(std::string(name));
  if (it == programs_.end()) return;
  if (it->second.handle) vm_->freeCode(it->second.handle);
  programs_.erase(it);
}

void EELContext::updateAudio(const AudioFeatures& audio, double engineTimeSeconds) {
  if (!vm_) return;
  const size_t sampleCount = EelVm::kLegacyVisSamples;
  const int channels = audio.oscR.empty() ? 1 : 2;
  oscBuffer_.assign(sampleCount * 2u, 128u);
  specBuffer_.assign(sampleCount * 2u, 0u);

  auto fillOscChannel = [&](const std::vector<float>& src, std::uint8_t* dst) {
    if (src.empty()) {
      std::fill(dst, dst + sampleCount, 128u);
      return;
    }
    for (size_t i = 0; i < sampleCount; ++i) {
      float s = src[std::min(i, src.size() - 1)];
      s = std::clamp(s, -1.0f, 1.0f);
      dst[i] = static_cast<std::uint8_t>(std::lround(s * 127.5f + 127.5f));
    }
  };

  auto fillSpecChannel = [&](const std::vector<float>& src, std::uint8_t* dst) {
    if (src.empty()) {
      std::fill(dst, dst + sampleCount, 0u);
      return;
    }
    for (size_t i = 0; i < sampleCount; ++i) {
      float s = src[std::min(i, src.size() - 1)];
      s = std::clamp(s, 0.0f, 1.0f);
      dst[i] = static_cast<std::uint8_t>(std::lround(s * 255.0f));
    }
  };

  fillOscChannel(audio.oscL, oscBuffer_.data());
  fillOscChannel(audio.oscR, oscBuffer_.data() + sampleCount);

  fillSpecChannel(audio.spectrum.left, specBuffer_.data());
  fillSpecChannel(audio.spectrum.right, specBuffer_.data() + sampleCount);

  if (channels < 2) {
    std::copy_n(oscBuffer_.data(), sampleCount, oscBuffer_.data() + sampleCount);
    std::copy_n(specBuffer_.data(), sampleCount, specBuffer_.data() + sampleCount);
  }

  EelVm::LegacySources sources{};
  sources.oscBase = oscBuffer_.data();
  sources.specBase = specBuffer_.data();
  sources.sampleCount = sampleCount;
  sources.channels = channels;
  sources.audioTimeSeconds = engineTimeSeconds;
  sources.engineTimeSeconds = engineTimeSeconds;
  vm_->setLegacySources(sources);
}

#else  // AVS_ENABLE_EEL2

EELContext::EELContext() = default;
EELContext::~EELContext() = default;

bool EELContext::isEnabled() const { return false; }

void EELContext::setVariable(std::string_view name, double value) {
  variables_[std::string(name)] = value;
}

double EELContext::getVariable(std::string_view name) const {
  auto it = variables_.find(std::string(name));
  return it != variables_.end() ? it->second : 0.0;
}

bool EELContext::compile(std::string_view name, std::string_view code) {
  programs_[std::string(name)] = std::string(code);
  return false;
}

bool EELContext::execute(std::string_view) { return false; }

void EELContext::remove(std::string_view name) { programs_.erase(std::string(name)); }

void EELContext::updateAudio(const AudioFeatures&, double) {}

#endif  // AVS_ENABLE_EEL2

}  // namespace avs

