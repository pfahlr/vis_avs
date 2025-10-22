#include "effects/render/effect_svp_loader.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "avs/core/RenderContext.hpp"

#if defined(_WIN32)
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

namespace avs::effects::render {
namespace {

#if defined(_WIN32)
using NativeHandle = HMODULE;

NativeHandle openLibrary(const std::filesystem::path& path) {
  return ::LoadLibraryW(path.wstring().c_str());
}

void closeLibrary(NativeHandle handle) {
  if (handle) {
    ::FreeLibrary(handle);
  }
}

void* resolveSymbol(NativeHandle handle, const char* name) {
  return handle ? reinterpret_cast<void*>(::GetProcAddress(handle, name)) : nullptr;
}

constexpr bool kPlatformSupportsSvp = true;
#else
using NativeHandle = void*;

NativeHandle openLibrary(const std::filesystem::path& path) {
  return dlopen(path.string().c_str(), RTLD_LAZY | RTLD_LOCAL);
}

void closeLibrary(NativeHandle handle) {
  if (handle) {
    dlclose(handle);
  }
}

void* resolveSymbol(NativeHandle handle, const char* name) {
  return handle ? dlsym(handle, name) : nullptr;
}

constexpr bool kPlatformSupportsSvp = false;
#endif

class ModuleHandle {
 public:
  ModuleHandle() = default;
  explicit ModuleHandle(NativeHandle handle) : handle_(handle) {}
  ~ModuleHandle() { reset(); }

  ModuleHandle(const ModuleHandle&) = delete;
  ModuleHandle& operator=(const ModuleHandle&) = delete;

  ModuleHandle(ModuleHandle&& other) noexcept : handle_(other.handle_) { other.handle_ = {}; }

  ModuleHandle& operator=(ModuleHandle&& other) noexcept {
    if (this != &other) {
      reset();
      handle_ = other.handle_;
      other.handle_ = {};
    }
    return *this;
  }

  void reset(NativeHandle handle = {}) {
    if (handle_) {
      closeLibrary(handle_);
    }
    handle_ = handle;
  }

  [[nodiscard]] bool valid() const noexcept { return handle_ != NativeHandle{}; }
  [[nodiscard]] NativeHandle get() const noexcept { return handle_; }

  [[nodiscard]] NativeHandle release() noexcept {
    NativeHandle handle = handle_;
    handle_ = {};
    return handle;
  }

 private:
  NativeHandle handle_{};
};

std::filesystem::path defaultSettingsPath(const std::filesystem::path& library) {
  auto directory = library.parent_path();
  if (directory.empty()) {
    directory = std::filesystem::current_path();
  }
  return directory / "avs.ini";
}

std::uint8_t clampToByte(float value) {
  const float clamped = std::clamp(value, 0.0f, 255.0f);
  return static_cast<std::uint8_t>(std::lround(clamped));
}

}  // namespace

SvpLoader::SvpLoader() = default;

SvpLoader::~SvpLoader() {
  std::lock_guard<std::mutex> lock(mutex_);
  unloadModuleLocked();
}

void SvpLoader::setParams(const avs::core::ParamBlock& params) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (params.contains("enabled")) {
    enabled_ = params.getBool("enabled", enabled_);
  }
  updateConfigurationLocked(params);
}

void SvpLoader::updateConfigurationLocked(const avs::core::ParamBlock& params) {
  static constexpr std::array<std::string_view, 4> kLibraryKeys = {
      std::string_view{"library"}, std::string_view{"file"}, std::string_view{"path"},
      std::string_view{"module"}};
  static constexpr std::array<std::string_view, 3> kBaseKeys = {std::string_view{"base_path"},
                                                                std::string_view{"preset_dir"},
                                                                std::string_view{"library_dir"}};

  bool baseChanged = false;
  std::filesystem::path newBase;
  for (auto key : kBaseKeys) {
    const std::string keyStr(key);
    if (!params.contains(keyStr)) {
      continue;
    }
    newBase = std::filesystem::path(params.getString(keyStr, ""));
    baseChanged = true;
    break;
  }
  if (baseChanged && baseDirectory_ != newBase) {
    baseDirectory_ = std::move(newBase);
    if (!requestedLibrary_.empty()) {
      needsReload_ = true;
    }
  }

  std::filesystem::path newLibrary;
  bool librarySpecified = false;
  for (auto key : kLibraryKeys) {
    const std::string keyStr(key);
    if (!params.contains(keyStr)) {
      continue;
    }
    librarySpecified = true;
    const std::string value = params.getString(keyStr, "");
    if (!value.empty()) {
      newLibrary = value;
      break;
    }
    newLibrary.clear();
    break;
  }

  if (!librarySpecified) {
    return;
  }

  if (newLibrary.empty()) {
    if (!requestedLibrary_.empty()) {
      requestedLibrary_.clear();
      needsReload_ = true;
    }
    return;
  }

  if (requestedLibrary_ != newLibrary) {
    requestedLibrary_ = std::move(newLibrary);
    needsReload_ = true;
  }
}

bool SvpLoader::render(avs::core::RenderContext& context) {
  if (!enabled_) {
    return true;
  }
  const auto width = std::max(context.width, 0);
  const auto height = std::max(context.height, 0);
  const std::size_t requiredSize =
      static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u;
  if (!context.framebuffer.data || context.framebuffer.size < requiredSize) {
    return true;
  }

  std::lock_guard<std::mutex> lock(mutex_);
  if (needsReload_) {
    reloadModuleLocked();
  }

  if (!moduleInfo_ || !moduleHandle_) {
    return true;
  }

  ensureInitializedLocked();

  LegacyVisData data{};
  prepareVisData(context, data);

  if (moduleInfo_->render) {
    moduleInfo_->render(reinterpret_cast<std::uint32_t*>(context.framebuffer.data), context.width,
                        context.height, context.width, &data);
  }

  return true;
}

void SvpLoader::reloadModuleLocked() {
  needsReload_ = false;

  if (!kPlatformSupportsSvp) {
    unloadModuleLocked();
    return;
  }

  if (requestedLibrary_.empty()) {
    unloadModuleLocked();
    return;
  }

  const std::filesystem::path resolved = resolveLibraryPath(requestedLibrary_);
  if (!loadModuleLocked(resolved)) {
    unloadModuleLocked();
  }
}

std::filesystem::path SvpLoader::resolveLibraryPath(const std::filesystem::path& candidate) const {
  if (candidate.is_absolute()) {
    return candidate;
  }
  if (!baseDirectory_.empty()) {
    return baseDirectory_ / candidate;
  }
  return std::filesystem::current_path() / candidate;
}

bool SvpLoader::loadModuleLocked(const std::filesystem::path& libraryPath) {
  ModuleHandle handle(openLibrary(libraryPath));
  if (!handle.valid()) {
    return false;
  }

  auto* rawQuery = reinterpret_cast<QueryModuleFn>(resolveSymbol(handle.get(), "QueryModule"));
  if (!rawQuery) {
    rawQuery = reinterpret_cast<QueryModuleFn>(
        resolveSymbol(handle.get(), "?QueryModule@@YAPAUUltraVisInfo@@XZ"));
  }
  if (!rawQuery) {
    return false;
  }

  LegacyVisInfo* info = rawQuery();
  if (!info || !info->render) {
    return false;
  }

  unloadModuleLocked();

  moduleHandle_ = handle.release();

  moduleInfo_ = info;
  loadedLibrary_ = libraryPath;
  settingsFile_ = defaultSettingsPath(libraryPath);
  initialized_ = false;

  applySettingsLocked(moduleInfo_->openSettings);
  ensureInitializedLocked();

  return true;
}

void SvpLoader::applySettingsLocked(int (*settingsFn)(char* fileName)) {
  if (!settingsFn || settingsFile_.empty()) {
    return;
  }

  const std::string utf8 = settingsFile_.string();
  std::vector<char> buffer(utf8.begin(), utf8.end());
  buffer.push_back('\0');
  settingsFn(buffer.data());
}

void SvpLoader::ensureInitializedLocked() {
  if (initialized_) {
    return;
  }
  if (moduleInfo_ && moduleInfo_->initialize) {
    moduleInfo_->initialize();
  }
  initialized_ = true;
}

void SvpLoader::unloadModuleLocked() {
  if (moduleInfo_) {
    applySettingsLocked(moduleInfo_->saveSettings);
  }

  moduleInfo_ = nullptr;

  if (moduleHandle_) {
    ModuleHandle handle(reinterpret_cast<NativeHandle>(moduleHandle_));
    handle.reset();
  }

  moduleHandle_ = nullptr;
  loadedLibrary_.clear();
  settingsFile_.clear();
  initialized_ = false;
}

void SvpLoader::prepareVisData(const avs::core::RenderContext& context, LegacyVisData& data) const {
  accumulatedMillis_ += context.deltaSeconds * 1000.0;
  if (accumulatedMillis_ < 0.0) {
    accumulatedMillis_ = 0.0;
  }
  const auto millis = static_cast<std::uint64_t>(accumulatedMillis_);
  data.millisecond = static_cast<std::uint32_t>(millis & 0xFFFFFFFFu);

  fillWaveform(context, data.waveform[0]);
  std::copy(std::begin(data.waveform[0]), std::end(data.waveform[0]), data.waveform[1]);
  fillSpectrum(context, data.spectrum[0]);
  std::copy(std::begin(data.spectrum[0]), std::end(data.spectrum[0]), data.spectrum[1]);
}

void SvpLoader::fillWaveform(const avs::core::RenderContext& context,
                             std::uint8_t (&buffer)[512]) const {
  std::fill(std::begin(buffer), std::end(buffer), static_cast<std::uint8_t>(128));
  if (!context.audioAnalysis) {
    return;
  }

  const auto& waveform = context.audioAnalysis->waveform;
  const std::size_t sourceSize = waveform.size();
  if (sourceSize == 0) {
    return;
  }
  const double lastIndex = static_cast<double>(sourceSize - 1);
  for (std::size_t i = 0; i < 512; ++i) {
    const double t = (sourceSize > 1) ? (static_cast<double>(i) / 511.0) * lastIndex : 0.0;
    const std::size_t index =
        std::min<std::size_t>(sourceSize - 1, static_cast<std::size_t>(std::lround(t)));
    const float sample = waveform[index];
    const float normalized = (sample + 1.0f) * 127.5f;
    buffer[i] = clampToByte(normalized);
  }
}

void SvpLoader::fillSpectrum(const avs::core::RenderContext& context,
                             std::uint8_t (&buffer)[256]) const {
  std::fill(std::begin(buffer), std::end(buffer), 0);

  const float* source = nullptr;
  std::size_t sourceSize = 0;

  if (context.audioSpectrum.data && context.audioSpectrum.size > 0) {
    source = context.audioSpectrum.data;
    sourceSize = context.audioSpectrum.size;
  } else if (context.audioAnalysis) {
    source = context.audioAnalysis->spectrum.data();
    sourceSize = context.audioAnalysis->spectrum.size();
  }

  if (!source || sourceSize == 0) {
    return;
  }

  const std::size_t stride = std::max<std::size_t>(1, sourceSize / 256);
  for (std::size_t i = 0; i < 256; ++i) {
    const std::size_t begin = i * stride;
    const std::size_t end = std::min(sourceSize, begin + stride);
    float peak = 0.0f;
    for (std::size_t s = begin; s < end; ++s) {
      peak = std::max(peak, source[s]);
    }
    const float scaled = peak * 255.0f;
    buffer[i] = clampToByte(scaled);
  }
}

}  // namespace avs::effects::render
