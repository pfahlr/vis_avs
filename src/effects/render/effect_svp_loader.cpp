#include "effects/render/effect_svp_loader.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <limits>
#include <system_error>

#include "audio/analyzer.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace avs::effects::render {
namespace {
constexpr double kFallbackFrameMs = 16.6666666667;

std::string trim(std::string_view value) {
  auto trimmed = value;
  while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.front()))) {
    trimmed.remove_prefix(1);
  }
  while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.back()))) {
    trimmed.remove_suffix(1);
  }
  if (trimmed.size() >= 2 && ((trimmed.front() == '"' && trimmed.back() == '"') ||
                              (trimmed.front() == '\'' && trimmed.back() == '\''))) {
    trimmed.remove_prefix(1);
    trimmed.remove_suffix(1);
  }
  return std::string(trimmed);
}

}  // namespace

SvpLoader::SvpLoader() {
  settingsFile_.fill(0);
  const char defaultSettings[] = "avs.ini";
  std::memcpy(settingsFile_.data(), defaultSettings, sizeof(defaultSettings));
}

SvpLoader::~SvpLoader() {
  std::lock_guard<std::mutex> lock(mutex_);
  unloadLibrary();
}

void SvpLoader::setParams(const avs::core::ParamBlock& params) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::string library = sanitizePath(params.getString("library", ""));
  if (library.empty()) {
    library = sanitizePath(params.getString("file", ""));
  }
  if (library.empty()) {
    library = sanitizePath(params.getString("path", ""));
  }
  if (library.empty()) {
    library = sanitizePath(params.getString("filename", ""));
  }
  if (library != desiredLibrary_) {
    requestReload(library);
  }
}

bool SvpLoader::render(avs::core::RenderContext& context) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (needsReload_) {
    ensureLibraryLoaded();
  }

#ifdef _WIN32
  if (!moduleInfo_ || !moduleInfo_->render || context.framebuffer.data == nullptr ||
      context.width <= 0 || context.height <= 0) {
    renderFallback(context);
    return true;
  }

  if (moduleInfo_->openSettings && !settingsOpened_) {
    settingsOpened_ = moduleInfo_->openSettings(settingsFile_.data()) != 0;
  }
  if (moduleInfo_->initialize && !initialized_) {
    moduleInfo_->initialize();
    initialized_ = true;
  }

  updateVisData(context, moduleInfo_->required);

  moduleInfo_->render(reinterpret_cast<unsigned long*>(context.framebuffer.data), context.width,
                      context.height, context.width, &visData_);
#else
  (void)context;
  renderFallback(context);
#endif
  return true;
}

std::string SvpLoader::sanitizePath(std::string_view value) { return trim(value); }

std::uint8_t SvpLoader::waveformSampleToByte(float sample) {
  const float normalized = std::clamp((sample + 1.0f) * 0.5f, 0.0f, 1.0f);
  return static_cast<std::uint8_t>(std::lround(normalized * 255.0f));
}

std::uint8_t SvpLoader::spectrumSampleToByte(float sample) {
  const float clamped = std::clamp(sample, 0.0f, 1.0f);
  return static_cast<std::uint8_t>(std::lround(clamped * 255.0f));
}

void SvpLoader::requestReload(const std::string& path) {
  desiredLibrary_ = path;
  needsReload_ = true;
  settingsOpened_ = false;
  initialized_ = false;
}

void SvpLoader::ensureLibraryLoaded() {
  needsReload_ = false;
  unloadLibrary();

  if (desiredLibrary_.empty()) {
    return;
  }

#ifdef _WIN32
  std::filesystem::path requested(desiredLibrary_);
  std::filesystem::path resolved = requested;
  std::error_code ec;
  const auto absolutePath = std::filesystem::absolute(requested, ec);
  if (!ec) {
    resolved = absolutePath;
  }

  HMODULE module = LoadLibraryW(resolved.wstring().c_str());
  if (!module) {
    module = LoadLibraryA(resolved.string().c_str());
  }
  if (!module) {
    return;
  }

  QueryModuleFn queryEntry = reinterpret_cast<QueryModuleFn>(GetProcAddress(module, "QueryModule"));
  if (!queryEntry) {
    queryEntry =
        reinterpret_cast<QueryModuleFn>(GetProcAddress(module, "?QueryModule@@YAPAUVisInfo@@XZ"));
  }
  if (!queryEntry) {
    FreeLibrary(module);
    return;
  }

  SvpVisInfo* info = queryEntry ? queryEntry() : nullptr;
  if (!info || !info->render) {
    FreeLibrary(module);
    return;
  }

  libraryHandle_ = module;
  moduleInfo_ = info;
#else
  // Non-Windows builds intentionally skip SVP loading.
#endif
}

void SvpLoader::unloadLibrary() {
#ifdef _WIN32
  if (moduleInfo_ && moduleInfo_->saveSettings && settingsOpened_) {
    moduleInfo_->saveSettings(settingsFile_.data());
  }
  moduleInfo_ = nullptr;
  if (libraryHandle_) {
    FreeLibrary(static_cast<HMODULE>(libraryHandle_));
    libraryHandle_ = nullptr;
  }
#else
  moduleInfo_ = nullptr;
  libraryHandle_ = nullptr;
#endif
  settingsOpened_ = false;
  initialized_ = false;
}

void SvpLoader::updateVisData(const avs::core::RenderContext& context, long requirements) {
  if (context.frameIndex == 0) {
    timeAccumulatorMs_ = 0.0;
  } else {
    const double deltaMs =
        context.deltaSeconds > 0.0 ? context.deltaSeconds * 1000.0 : kFallbackFrameMs;
    timeAccumulatorMs_ = std::clamp(timeAccumulatorMs_ + deltaMs, 0.0,
                                    static_cast<double>(std::numeric_limits<unsigned long>::max()));
  }
  visData_.millSec = static_cast<unsigned long>(std::lround(timeAccumulatorMs_));

  const bool needWaveform = (requirements & kRequireWaveform) != 0;
  const bool needSpectrum = (requirements & kRequireSpectrum) != 0;

  for (auto& channel : visData_.waveform) {
    std::fill(std::begin(channel), std::end(channel), static_cast<std::uint8_t>(128));
  }
  for (auto& channel : visData_.spectrum) {
    std::fill(std::begin(channel), std::end(channel), static_cast<std::uint8_t>(0));
  }

  const avs::audio::Analysis* analysis = context.audioAnalysis;
  if (!analysis) {
    return;
  }

  if (needWaveform) {
    const std::size_t sampleCount = std::min<std::size_t>(512, analysis->waveform.size());
    for (std::size_t i = 0; i < sampleCount; ++i) {
      const std::uint8_t value = waveformSampleToByte(analysis->waveform[i]);
      visData_.waveform[0][i] = value;
      visData_.waveform[1][i] = value;
    }
  }

  if (needSpectrum) {
    const std::size_t sampleCount = std::min<std::size_t>(256, analysis->spectrum.size());
    for (std::size_t i = 0; i < sampleCount; ++i) {
      const std::uint8_t value = spectrumSampleToByte(analysis->spectrum[i]);
      visData_.spectrum[0][i] = value;
      visData_.spectrum[1][i] = value;
    }
  }
}

void SvpLoader::renderFallback(avs::core::RenderContext& context) {
  (void)context;
  // Intentionally left blank: the legacy loader becomes a no-op when SVP
  // plug-ins are unavailable, matching the behaviour of historical builds.
}

}  // namespace avs::effects::render
