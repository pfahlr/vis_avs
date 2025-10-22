#include "effects/render/effect_svp_loader.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <system_error>
#include <vector>

#include "audio/analyzer.h"

#if defined(_WIN32)
#include <windows.h>
#else
#  if defined(__has_include)
#    if __has_include(<dlfcn.h>)
#      define AVS_SVP_HAVE_DLOPEN 1
#    endif
#  endif
#  ifndef AVS_SVP_HAVE_DLOPEN
#    if defined(__unix__) || defined(__APPLE__)
#      define AVS_SVP_HAVE_DLOPEN 1
#    else
#      define AVS_SVP_HAVE_DLOPEN 0
#    endif
#  endif
#  if AVS_SVP_HAVE_DLOPEN
#    include <dlfcn.h>
#  endif
#endif

namespace avs::effects::render {
namespace {

#if defined(_WIN32)
void* openLibrary(const std::string& path) {
  std::filesystem::path fsPath(path);
  std::wstring wide = fsPath.wstring();
  HMODULE module = ::LoadLibraryW(wide.c_str());
  return reinterpret_cast<void*>(module);
}

void closeLibrary(void* handle) {
  if (handle) {
    ::FreeLibrary(reinterpret_cast<HMODULE>(handle));
  }
}

void* resolveSymbol(void* handle, const char* name) {
  if (!handle) {
    return nullptr;
  }
  return reinterpret_cast<void*>(
      ::GetProcAddress(reinterpret_cast<HMODULE>(handle), name));
}
#else

void* openLibrary(const std::string& path) {
#if AVS_SVP_HAVE_DLOPEN
  return ::dlopen(path.c_str(), RTLD_LAZY | RTLD_LOCAL);
#else
  (void)path;
  return nullptr;
#endif
}

void closeLibrary(void* handle) {
#if AVS_SVP_HAVE_DLOPEN
  if (handle) {
    ::dlclose(handle);
  }
#else
  (void)handle;
#endif
}

void* resolveSymbol(void* handle, const char* name) {
#if AVS_SVP_HAVE_DLOPEN
  if (!handle) {
    return nullptr;
  }
  return ::dlsym(handle, name);
#else
  (void)handle;
  (void)name;
  return nullptr;
#endif
}

#endif  // defined(_WIN32)

std::uint8_t clampToByte(float value) {
  value = std::clamp(value, 0.0f, 255.0f);
  return static_cast<std::uint8_t>(std::lround(value));
}

}  // namespace

SvpLoader::SvpLoader() = default;

SvpLoader::~SvpLoader() {
  std::lock_guard<std::mutex> lock(mutex_);
  unloadLibraryLocked();
}

void SvpLoader::setParams(const avs::core::ParamBlock& params) {
  std::string requested = params.getString("library");
  if (requested.empty()) {
    requested = params.getString("path");
  }
  if (requested.empty()) {
    requested = params.getString("file");
  }

  std::lock_guard<std::mutex> lock(mutex_);
  if (requested != requestedLibrary_) {
    requestedLibrary_ = requested;
    libraryDirty_ = true;
  } else if (!requested.empty() && !libraryHandle_) {
    libraryDirty_ = true;
  }
}

bool SvpLoader::render(avs::core::RenderContext& context) {
  std::lock_guard<std::mutex> lock(mutex_);
  reloadLibraryLocked();

  if (!visInfo_ || !visInfo_->render) {
    return true;
  }
  if (!hasFramebuffer(context)) {
    return true;
  }

  LegacyVisData data{};
  populateVisData(context, data);

  auto* pixels = reinterpret_cast<std::uint32_t*>(context.framebuffer.data);
  const int pitch = context.width;
  const int result = visInfo_->render(pixels, context.width, context.height, pitch, &data);
  (void)result;
  return true;
}

void SvpLoader::reloadLibraryLocked() {
  if (!libraryDirty_) {
    return;
  }
  libraryDirty_ = false;

  unloadLibraryLocked();
  if (requestedLibrary_.empty()) {
    return;
  }

  const std::string resolved = resolveLibraryPath(requestedLibrary_);
  if (resolved.empty()) {
    return;
  }

  void* handle = openLibrary(resolved);
  if (!handle) {
    return;
  }

  void* symbol = resolveSymbol(handle, "QueryModule");
  if (!symbol) {
    symbol = resolveSymbol(handle, "?QueryModule@@YAPAUUltraVisInfo@@XZ");
  }
  if (!symbol) {
    closeLibrary(handle);
    return;
  }

  queryModule_ = reinterpret_cast<QueryModuleFn>(symbol);
  LegacyVisInfo* info = queryModule_ ? queryModule_() : nullptr;
  if (!info || !info->render) {
    queryModule_ = nullptr;
    closeLibrary(handle);
    return;
  }

  libraryHandle_ = handle;
  visInfo_ = info;
  loadedLibraryPath_ = resolved;
  updateConfigBuffer(resolved);

  if (visInfo_->openSettings && !configFileBuffer_.empty()) {
    visInfo_->openSettings(configFileBuffer_.data());
  }
  if (visInfo_->initialize) {
    visInfo_->initialize();
  }
  startTimeInitialized_ = false;
}

void SvpLoader::unloadLibraryLocked() {
  if (visInfo_ && visInfo_->saveSettings && !configFileBuffer_.empty()) {
    visInfo_->saveSettings(configFileBuffer_.data());
  }
  if (libraryHandle_) {
    closeLibrary(libraryHandle_);
  }
  libraryHandle_ = nullptr;
  visInfo_ = nullptr;
  queryModule_ = nullptr;
  loadedLibraryPath_.clear();
  configFileBuffer_.clear();
}

std::string SvpLoader::resolveLibraryPath(const std::string& candidate) const {
  namespace fs = std::filesystem;
  if (candidate.empty()) {
    return {};
  }

  auto addAttempt = [](const fs::path& base, std::vector<fs::path>& attempts) {
    if (base.empty()) {
      return;
    }
    attempts.push_back(base);
    if (base.extension().empty()) {
      attempts.push_back(base.string() + ".svp");
      attempts.push_back(base.string() + ".uvs");
      attempts.push_back(base.string() + ".dll");
    }
  };

  std::vector<fs::path> attempts;
  fs::path raw(candidate);
  std::error_code ec;
  const fs::path cwd = fs::current_path(ec);

  if (raw.is_absolute()) {
    addAttempt(raw, attempts);
  } else {
    if (!cwd.empty()) {
      addAttempt(cwd / raw, attempts);
      addAttempt(cwd / "resources" / "svp" / raw, attempts);
      addAttempt(cwd / "resources" / "plugins" / raw, attempts);
    }
    addAttempt(raw, attempts);
  }

  for (const fs::path& attempt : attempts) {
    if (attempt.empty()) {
      continue;
    }
    std::error_code existsEc;
    if (fs::exists(attempt, existsEc) && !fs::is_directory(attempt, existsEc)) {
      std::error_code absoluteEc;
      fs::path absolute = fs::absolute(attempt, absoluteEc);
      if (absolute.empty() || absoluteEc) {
        return attempt.string();
      }
      return absolute.string();
    }
  }

  return {};
}

void SvpLoader::updateConfigBuffer(const std::string& libraryPath) {
  configFileBuffer_.clear();
  if (libraryPath.empty()) {
    return;
  }
  std::filesystem::path path(libraryPath);
  std::filesystem::path ini = path.parent_path() / "avs.ini";
  const std::string iniString = ini.string();
  configFileBuffer_.assign(iniString.begin(), iniString.end());
  configFileBuffer_.push_back('\0');
}

void SvpLoader::populateVisData(const avs::core::RenderContext& context, LegacyVisData& data) {
  for (auto& channel : data.waveform) {
    channel.fill(128);
  }
  for (auto& channel : data.spectrum) {
    channel.fill(0);
  }

  const auto now = std::chrono::steady_clock::now();
  if (!startTimeInitialized_) {
    startTime_ = now;
    startTimeInitialized_ = true;
  }
  data.millisecond = static_cast<std::uint32_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime_).count());

  const avs::audio::Analysis* analysis = context.audioAnalysis;
  if (analysis) {
    const auto& waveform = analysis->waveform;
    const std::size_t copyCount = std::min<std::size_t>(waveform.size(), data.waveform[0].size());
    for (std::size_t i = 0; i < copyCount; ++i) {
      const std::uint8_t sample = convertWaveformSample(waveform[i]);
      data.waveform[0][i] = sample;
      data.waveform[1][i] = sample;
    }
  }

  const float* spectrumData = nullptr;
  std::size_t spectrumSize = 0;
  if (context.audioSpectrum.data && context.audioSpectrum.size > 0) {
    spectrumData = context.audioSpectrum.data;
    spectrumSize = context.audioSpectrum.size;
  } else if (analysis) {
    spectrumData = analysis->spectrum.data();
    spectrumSize = analysis->spectrum.size();
  }

  const std::size_t maxBins = data.spectrum[0].size();
  for (std::size_t i = 0; i < maxBins; ++i) {
    const std::size_t idx = i * 2;
    float value = 0.0f;
    if (spectrumData && idx < spectrumSize) {
      value = spectrumData[idx];
      if (idx + 1 < spectrumSize) {
        value = (value + spectrumData[idx + 1]) * 0.5f;
      }
    }
    const std::uint8_t sample = convertSpectrumSample(value);
    data.spectrum[0][i] = sample;
    data.spectrum[1][i] = sample;
  }
}

std::uint8_t SvpLoader::convertWaveformSample(float value) {
  if (!std::isfinite(value)) {
    value = 0.0f;
  }
  value = std::clamp(value, -1.0f, 1.0f);
  const float scaled = (value + 1.0f) * 127.5f;
  return clampToByte(scaled);
}

std::uint8_t SvpLoader::convertSpectrumSample(float value) {
  if (!std::isfinite(value)) {
    value = 0.0f;
  }
  value = std::max(0.0f, value);
  const float scaled = value * 4.0f;
  return clampToByte(scaled);
}

bool SvpLoader::hasFramebuffer(const avs::core::RenderContext& context) {
  if (context.width <= 0 || context.height <= 0) {
    return false;
  }
  if (!context.framebuffer.data) {
    return false;
  }
  const std::size_t required = static_cast<std::size_t>(context.width) *
                               static_cast<std::size_t>(context.height) * 4u;
  return context.framebuffer.size >= required;
}

}  // namespace avs::effects::render

