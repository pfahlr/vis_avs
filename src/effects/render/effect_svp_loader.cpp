#include "effects/render/effect_svp_loader.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <string_view>
#include <system_error>
#include <vector>

#include "audio/analyzer.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace avs::effects::render {
namespace {
constexpr std::uint32_t kWaveformFlag = 0x0001u;
constexpr std::uint32_t kSpectrumFlag = 0x0002u;
constexpr double kDefaultDeltaMilliseconds = 16.0;

float sampleLinear(const float* data, std::size_t size, float position) {
  if (!data || size == 0) {
    return 0.0f;
  }
  if (position <= 0.0f) {
    return data[0];
  }
  const float maxIndex = static_cast<float>(size - 1);
  if (position >= maxIndex) {
    return data[size - 1];
  }
  const std::size_t index = static_cast<std::size_t>(position);
  const float fraction = position - static_cast<float>(index);
  const std::size_t nextIndex = std::min(index + 1, size - 1);
  return data[index] + (data[nextIndex] - data[index]) * fraction;
}

}  // namespace

SvpLoader::SvpLoader() = default;

SvpLoader::~SvpLoader() {
  std::lock_guard<std::mutex> lock(mutex_);
  unloadLibrary();
}

void SvpLoader::setParams(const avs::core::ParamBlock& params) {
  std::string path;
  const auto tryExtract = [&params](std::string_view key) -> std::string {
    const std::string keyString(key);
    if (!params.contains(keyString)) {
      return {};
    }
    return params.getString(keyString, "");
  };

  path = tryExtract("library");
  if (path.empty()) {
    path = tryExtract("file");
  }
  if (path.empty()) {
    path = tryExtract("path");
  }
  if (path.empty()) {
    path = tryExtract("svp");
  }
  if (path.empty()) {
    path = tryExtract("filename");
  }

  requestLibraryReload(path);
}

bool SvpLoader::render(avs::core::RenderContext& context) {
  std::lock_guard<std::mutex> lock(mutex_);
  ensureLibraryLoaded();

  if (!visInfo_ || !visInfo_->render) {
    return true;
  }

  if (!hasValidFramebuffer(context)) {
    return true;
  }

  const std::size_t pixelCount =
      static_cast<std::size_t>(context.width) * static_cast<std::size_t>(context.height);
  ensurePixelBuffer(pixelCount);

  for (std::size_t i = 0; i < pixelCount; ++i) {
    const std::size_t offset = i * 4u;
    pixelBuffer_[i] = rgbaToAbgr(context.framebuffer.data + offset);
  }

  prepareVisData(context);

  const int pitch = context.width;
  const int rendered =
      visInfo_->render(pixelBuffer_.data(), context.width, context.height, pitch, &visData_);

  for (std::size_t i = 0; i < pixelCount; ++i) {
    const std::size_t offset = i * 4u;
    abgrToRgba(pixelBuffer_[i], context.framebuffer.data + offset);
  }

  return rendered != 0;
}

void SvpLoader::requestLibraryReload(const std::string& path) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (path == requestedLibrary_) {
    return;
  }
  requestedLibrary_ = path;
  libraryDirty_ = true;
}

void SvpLoader::unloadLibrary() {
  if (visInfo_ && visInfo_->saveSettings && !settingsPath_.empty()) {
    visInfo_->saveSettings(settingsPath_.empty() ? nullptr : settingsPath_.data());
  }

  visInfo_ = nullptr;
  settingsPath_.clear();

  if (!libraryHandle_) {
    return;
  }

#ifdef _WIN32
  FreeLibrary(static_cast<HMODULE>(libraryHandle_));
#else
  dlclose(libraryHandle_);
#endif
  libraryHandle_ = nullptr;
  resolvedLibrary_.clear();
}

void SvpLoader::ensureLibraryLoaded() {
  if (!libraryDirty_) {
    return;
  }

  unloadLibrary();
  libraryDirty_ = false;

  if (requestedLibrary_.empty()) {
    return;
  }

  const std::filesystem::path resolved = resolveLibraryPath(requestedLibrary_);
  if (resolved.empty()) {
    return;
  }

  if (loadLibrary(resolved)) {
    resolvedLibrary_ = resolved;
    settingsPath_ = (resolved.parent_path() / "avs.ini").string();
    if (visInfo_ && visInfo_->openSettings && !settingsPath_.empty()) {
      visInfo_->openSettings(settingsPath_.data());
    }
    if (visInfo_ && visInfo_->initialize) {
      visInfo_->initialize();
    }
  }
}

bool SvpLoader::loadLibrary(const std::filesystem::path& path) {
#ifdef _WIN32
  HMODULE module = LoadLibraryW(path.c_str());
  if (!module) {
    return false;
  }
  libraryHandle_ = static_cast<LibraryHandle>(module);
  QueryModuleFn query = reinterpret_cast<QueryModuleFn>(GetProcAddress(module, "QueryModule"));
  if (!query) {
    query =
        reinterpret_cast<QueryModuleFn>(GetProcAddress(module, "?QueryModule@@YAPAUVisInfo@@XZ"));
  }
#else
  void* module = dlopen(path.c_str(), RTLD_LAZY);
  if (!module) {
    return false;
  }
  libraryHandle_ = module;
  QueryModuleFn query = reinterpret_cast<QueryModuleFn>(dlsym(module, "QueryModule"));
#endif
  if (!query) {
    unloadLibrary();
    return false;
  }

  VisInfo* info = query();
  if (!info || !info->render) {
    unloadLibrary();
    return false;
  }

  visInfo_ = info;
  return true;
}

bool SvpLoader::hasValidFramebuffer(const avs::core::RenderContext& context) const {
  if (!context.framebuffer.data || context.width <= 0 || context.height <= 0) {
    return false;
  }
  const std::size_t required =
      static_cast<std::size_t>(context.width) * static_cast<std::size_t>(context.height) * 4u;
  return context.framebuffer.size >= required;
}

void SvpLoader::ensurePixelBuffer(std::size_t pixelCount) {
  if (pixelBuffer_.size() < pixelCount) {
    pixelBuffer_.assign(pixelCount, 0u);
  }
}

void SvpLoader::prepareVisData(const avs::core::RenderContext& context) {
  const double delta =
      context.deltaSeconds > 0.0 ? context.deltaSeconds * 1000.0 : kDefaultDeltaMilliseconds;
  elapsedMilliseconds_ += delta;
  if (elapsedMilliseconds_ < 0.0) {
    elapsedMilliseconds_ = 0.0;
  }
  if (elapsedMilliseconds_ > static_cast<double>(std::numeric_limits<std::uint32_t>::max())) {
    elapsedMilliseconds_ = static_cast<double>(std::numeric_limits<std::uint32_t>::max());
  }
  visData_.millisecond = static_cast<std::uint32_t>(elapsedMilliseconds_);

  updateWaveform(context);
  updateSpectrum(context);
}

void SvpLoader::updateWaveform(const avs::core::RenderContext& context) {
  if (!needsWaveform()) {
    for (auto& channel : visData_.waveform) {
      channel.fill(128u);
    }
    return;
  }

  const avs::audio::Analysis* analysis = context.audioAnalysis;
  const float* waveformData = nullptr;
  std::size_t waveformSize = 0;
  if (analysis) {
    waveformData = analysis->waveform.data();
    waveformSize = analysis->waveform.size();
  }

  for (auto& channel : visData_.waveform) {
    channel.fill(128u);
  }

  if (!waveformData || waveformSize == 0) {
    return;
  }

  for (std::size_t i = 0; i < visData_.waveform[0].size(); ++i) {
    const float position = static_cast<float>(i) * static_cast<float>(waveformSize) /
                           static_cast<float>(visData_.waveform[0].size());
    const float sample = sampleLinear(waveformData, waveformSize, position);
    const float scaled = sample * 127.0f + 128.0f;
    const long rounded = std::lround(scaled);
    const auto value = static_cast<std::uint8_t>(std::clamp<long>(rounded, 0, 255));
    visData_.waveform[0][i] = value;
    visData_.waveform[1][i] = value;
  }
}

void SvpLoader::updateSpectrum(const avs::core::RenderContext& context) {
  if (!needsSpectrum()) {
    for (auto& channel : visData_.spectrum) {
      channel.fill(0u);
    }
    return;
  }

  const float* spectrumData = nullptr;
  std::size_t spectrumSize = 0;
  if (context.audioSpectrum.data && context.audioSpectrum.size > 0) {
    spectrumData = context.audioSpectrum.data;
    spectrumSize = context.audioSpectrum.size;
  } else if (context.audioAnalysis) {
    spectrumData = context.audioAnalysis->spectrum.data();
    spectrumSize = context.audioAnalysis->spectrum.size();
  }

  for (auto& channel : visData_.spectrum) {
    channel.fill(0u);
  }

  if (!spectrumData || spectrumSize == 0) {
    return;
  }

  for (std::size_t i = 0; i < visData_.spectrum[0].size(); ++i) {
    const float position = static_cast<float>(i) * static_cast<float>(spectrumSize) /
                           static_cast<float>(visData_.spectrum[0].size());
    const float sample = sampleLinear(spectrumData, spectrumSize, position);
    const float scaled = sample * 255.0f;
    const long rounded = std::lround(scaled);
    const auto value = static_cast<std::uint8_t>(std::clamp<long>(rounded, 0, 255));
    visData_.spectrum[0][i] = value;
    visData_.spectrum[1][i] = value;
  }
}

std::uint32_t SvpLoader::rgbaToAbgr(const std::uint8_t* pixel) {
  const std::uint32_t r = static_cast<std::uint32_t>(pixel[0]);
  const std::uint32_t g = static_cast<std::uint32_t>(pixel[1]);
  const std::uint32_t b = static_cast<std::uint32_t>(pixel[2]);
  const std::uint32_t a = static_cast<std::uint32_t>(pixel[3]);
  return (a << 24u) | (r << 16u) | (g << 8u) | b;
}

void SvpLoader::abgrToRgba(std::uint32_t value, std::uint8_t* pixel) {
  pixel[0] = static_cast<std::uint8_t>((value >> 16u) & 0xFFu);
  pixel[1] = static_cast<std::uint8_t>((value >> 8u) & 0xFFu);
  pixel[2] = static_cast<std::uint8_t>(value & 0xFFu);
  pixel[3] = static_cast<std::uint8_t>((value >> 24u) & 0xFFu);
}

std::filesystem::path SvpLoader::resolveLibraryPath(const std::string& requested) {
  if (requested.empty()) {
    return {};
  }

  std::vector<std::filesystem::path> candidates;
  const std::filesystem::path initial(requested);
  candidates.push_back(initial);
  if (!initial.has_extension()) {
    candidates.emplace_back(requested + ".svp");
    candidates.emplace_back(requested + ".uvs");
    candidates.emplace_back(requested + ".dll");
  }

  std::error_code ec;
  const std::filesystem::path current = std::filesystem::current_path(ec);

  for (auto candidate : candidates) {
    if (candidate.is_relative() && !current.empty()) {
      candidate = current / candidate;
    }
    std::error_code existsError;
    if (std::filesystem::exists(candidate, existsError) && !existsError) {
      std::error_code canonicalError;
      auto canonical = std::filesystem::weakly_canonical(candidate, canonicalError);
      if (!canonicalError) {
        return canonical;
      }
      return candidate;
    }
  }

  return {};
}

bool SvpLoader::needsWaveform() const {
  return visInfo_ && (static_cast<std::uint32_t>(visInfo_->required) & kWaveformFlag) != 0u;
}

bool SvpLoader::needsSpectrum() const {
  return visInfo_ && (static_cast<std::uint32_t>(visInfo_->required) & kSpectrumFlag) != 0u;
}

}  // namespace avs::effects::render
