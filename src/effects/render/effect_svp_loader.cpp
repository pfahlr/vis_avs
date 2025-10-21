#include "effects/render/effect_svp_loader.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "audio/analyzer.h"
#include "avs/core/ParamBlock.hpp"
#include "avs/core/RenderContext.hpp"

namespace {

constexpr std::string_view kEffectName = "Render / SVP Loader";

constexpr char kEnvSearchPath[] = "VIS_AVS_SVP_PATHS";

constexpr char kPathDelimiter =
#if defined(_WIN32)
    ';';
#else
    ':';
#endif

#if defined(_WIN32)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct SvpPluginInfo {
  unsigned long reserved = 0;
  char* pluginName = nullptr;
  long required = 0;
  void(__cdecl* initialize)() = nullptr;
  BOOL(__cdecl* render)(unsigned long*, int, int, int, void*) = nullptr;
  BOOL(__cdecl* saveSettings)(char*) = nullptr;
  BOOL(__cdecl* openSettings)(char*) = nullptr;
};
#endif

}  // namespace

Effect_RenderSvpLoader::Effect_RenderSvpLoader() { clearVisData(); }

Effect_RenderSvpLoader::~Effect_RenderSvpLoader() { unloadPlugin(); }

void Effect_RenderSvpLoader::setParams(const avs::core::ParamBlock& params) {
  applyParameters(params);
}

void Effect_RenderSvpLoader::applyParameters(const avs::core::ParamBlock& params) {
  std::string library = params.getString("library");
  if (library.empty()) {
    library = params.getString("file");
  }
  if (library.empty()) {
    library = params.getString("path");
  }
  if (library.empty()) {
    library = params.getString("filename");
  }
  if (library != requestedLibrary_) {
    requestedLibrary_ = std::move(library);
    libraryDirty_ = true;
  }
}

bool Effect_RenderSvpLoader::render(avs::core::RenderContext& context) {
  if (libraryDirty_) {
    unloadPlugin();
    libraryDirty_ = false;
    attemptedLoad_ = false;
    loadSuccessful_ = false;
    missingLibraryLogged_ = false;
    unsupportedLogged_ = false;
  }

  if (!ensureFramebuffer(context)) {
    return true;
  }

  if (!ensurePluginLoaded()) {
    return true;
  }

  updateVisData(context);

#if defined(_WIN32)
  auto* info = reinterpret_cast<SvpPluginInfo*>(pluginInfoHandle_);
  if (!info || !info->render) {
    return true;
  }

  auto* framebuffer = reinterpret_cast<unsigned long*>(context.framebuffer.data);
  const int pitch = context.width;
  info->render(framebuffer, context.width, context.height, pitch, &visData_);
#endif

  return true;
}

bool Effect_RenderSvpLoader::ensureFramebuffer(const avs::core::RenderContext& context) {
  if (context.width <= 0 || context.height <= 0 || !context.framebuffer.data) {
    if (!missingFramebufferLogged_) {
      std::clog << kEffectName << ": missing framebuffer, skipping SVP render" << std::endl;
      missingFramebufferLogged_ = true;
    }
    return false;
  }

  const std::size_t requiredBytes =
      static_cast<std::size_t>(context.width) * static_cast<std::size_t>(context.height) * 4u;
  if (context.framebuffer.size < requiredBytes) {
    if (!missingFramebufferLogged_) {
      std::clog << kEffectName << ": framebuffer too small for SVP output, skipping (have "
                << context.framebuffer.size << " bytes, need " << requiredBytes << ")" << std::endl;
      missingFramebufferLogged_ = true;
    }
    return false;
  }

  return true;
}

bool Effect_RenderSvpLoader::ensurePluginLoaded() {
  if (loadSuccessful_) {
    return true;
  }

  if (requestedLibrary_.empty()) {
    if (!missingLibraryLogged_) {
      std::clog << kEffectName << ": no SVP library configured" << std::endl;
      missingLibraryLogged_ = true;
    }
    return false;
  }

  if (!attemptedLoad_) {
    attemptedLoad_ = true;
    loadSuccessful_ = tryLoadPlugin();
    if (!loadSuccessful_) {
      if (!lastErrorMessage_.empty() && !missingLibraryLogged_) {
        std::clog << kEffectName << ": " << lastErrorMessage_ << std::endl;
        missingLibraryLogged_ = true;
      }
    }
  }

  return loadSuccessful_;
}

bool Effect_RenderSvpLoader::tryLoadPlugin() {
  lastErrorMessage_.clear();

  const auto resolved = resolveLibraryPath();
  if (!resolved) {
    lastErrorMessage_ = "could not locate '" + requestedLibrary_ + "'";
    return false;
  }

  return tryLoadPluginFromPath(*resolved);
}

bool Effect_RenderSvpLoader::tryLoadPluginFromPath(const std::filesystem::path& path) {
  loadedLibraryPath_.clear();

#if defined(_WIN32)
  const std::wstring widePath = path.wstring();
  HMODULE module = LoadLibraryW(widePath.c_str());
  if (!module) {
    lastErrorMessage_ = "failed to load '" + path.string() + "'";
    return false;
  }

  using QueryModuleFn = SvpPluginInfo*(__cdecl*)();
  auto* query = reinterpret_cast<QueryModuleFn>(GetProcAddress(module, "QueryModule"));
  if (!query) {
    query =
        reinterpret_cast<QueryModuleFn>(GetProcAddress(module, "?QueryModule@@YAPAUVisInfo@@XZ"));
  }

  if (!query) {
    FreeLibrary(module);
    lastErrorMessage_ = "missing QueryModule() export in '" + path.string() + "'";
    return false;
  }

  SvpPluginInfo* info = query();
  if (!info || !info->render) {
    FreeLibrary(module);
    lastErrorMessage_ = "invalid SVP module '" + path.string() + "'";
    return false;
  }

  moduleHandle_ = module;
  pluginInfoHandle_ = info;
  loadedLibraryPath_ = path;

  if (info->initialize) {
    info->initialize();
  }

  lastErrorMessage_.clear();
  return true;
#else
  (void)path;
  lastErrorMessage_ = "SVP playback requires Windows-compatible plugins";
  if (!unsupportedLogged_) {
    std::clog << kEffectName << ": " << lastErrorMessage_ << std::endl;
    unsupportedLogged_ = true;
    missingLibraryLogged_ = true;
  }
  return false;
#endif
}

void Effect_RenderSvpLoader::unloadPlugin() {
#if defined(_WIN32)
  if (moduleHandle_) {
    FreeLibrary(reinterpret_cast<HMODULE>(moduleHandle_));
  }
#endif
  moduleHandle_ = nullptr;
  pluginInfoHandle_ = nullptr;
  loadedLibraryPath_.clear();
}

std::optional<std::filesystem::path> Effect_RenderSvpLoader::resolveLibraryPath() const {
  namespace fs = std::filesystem;

  if (requestedLibrary_.empty()) {
    return std::nullopt;
  }

  const fs::path base(requestedLibrary_);
  std::vector<fs::path> candidates;
  candidates.push_back(base);
  if (!base.has_extension()) {
    candidates.emplace_back(requestedLibrary_ + ".svp");
    candidates.emplace_back(requestedLibrary_ + ".uvs");
  }

  auto exists = [](const fs::path& candidate) -> std::optional<fs::path> {
    std::error_code ec;
    if (candidate.empty()) {
      return std::nullopt;
    }
    if (fs::exists(candidate, ec) && !ec) {
      return fs::weakly_canonical(candidate, ec);
    }
    return std::nullopt;
  };

  std::vector<fs::path> searchDirs;
  searchDirs.emplace_back(fs::path("."));
  searchDirs.emplace_back(fs::path("resources"));
  searchDirs.emplace_back(fs::path("resources/svp"));

  if (const char* env = std::getenv(kEnvSearchPath)) {
    std::string value(env);
    std::size_t start = 0;
    while (start <= value.size()) {
      const std::size_t end = value.find(kPathDelimiter, start);
      const std::string_view piece(value.c_str() + start,
                                   (end == std::string::npos ? value.size() : end) - start);
      if (!piece.empty()) {
        searchDirs.emplace_back(fs::path(piece));
      }
      if (end == std::string::npos) {
        break;
      }
      start = end + 1;
    }
  }

  for (const auto& candidate : candidates) {
    if (candidate.is_absolute()) {
      if (auto resolved = exists(candidate)) {
        return resolved;
      }
      continue;
    }

    if (auto resolved = exists(candidate)) {
      return resolved;
    }

    for (const auto& dir : searchDirs) {
      auto combined = dir / candidate;
      if (auto resolved = exists(combined)) {
        return resolved;
      }
    }
  }

  return std::nullopt;
}

void Effect_RenderSvpLoader::clearVisData() {
  visData_.millis = 0;
  std::fill_n(&visData_.waveform[0][0], 2 * kWaveformSamples, static_cast<std::uint8_t>(128));
  std::fill_n(&visData_.spectrum[0][0], 2 * kSpectrumSamples, static_cast<std::uint8_t>(0));
}

void Effect_RenderSvpLoader::updateVisData(const avs::core::RenderContext& context) {
  const double millis = static_cast<double>(context.frameIndex) * context.deltaSeconds * 1000.0;
  const double clamped =
      std::clamp(millis, 0.0, static_cast<double>(std::numeric_limits<std::uint32_t>::max()));
  visData_.millis = static_cast<std::uint32_t>(clamped);

  const auto* analysis = context.audioAnalysis;
  if (!analysis) {
    return;
  }

  const auto& waveform = analysis->waveform;
  const auto& spectrum = analysis->spectrum;

  for (std::size_t i = 0; i < kWaveformSamples; ++i) {
    const double position = (kWaveformSamples > 1) ? (static_cast<double>(i) *
                                                      (avs::audio::Analysis::kWaveformSize - 1) /
                                                      static_cast<double>(kWaveformSamples - 1))
                                                   : 0.0;
    const std::size_t index0 = static_cast<std::size_t>(position);
    const std::size_t index1 = std::min(index0 + 1, avs::audio::Analysis::kWaveformSize - 1);
    const double t = position - static_cast<double>(index0);
    const float sample = static_cast<float>((1.0 - t) * waveform[index0] + t * waveform[index1]);
    const float scaled = (sample + 1.0f) * 127.5f;
    const auto byteValue = static_cast<std::uint8_t>(
        std::clamp<std::int32_t>(static_cast<std::int32_t>(std::lround(scaled)), 0, 255));
    visData_.waveform[0][i] = byteValue;
    visData_.waveform[1][i] = byteValue;
  }

  float maxMagnitude = 0.0f;
  for (float value : spectrum) {
    maxMagnitude = std::max(maxMagnitude, value);
  }
  const float logDenominator = maxMagnitude > 0.0f ? std::log1p(maxMagnitude) : 1.0f;

  for (std::size_t i = 0; i < kSpectrumSamples; ++i) {
    const std::size_t index0 =
        std::min<std::size_t>(i * 2, avs::audio::Analysis::kSpectrumSize - 1);
    const std::size_t index1 =
        std::min<std::size_t>(index0 + 1, avs::audio::Analysis::kSpectrumSize - 1);
    const float averaged = 0.5f * (spectrum[index0] + spectrum[index1]);
    const float normalized = logDenominator > 0.0f ? std::log1p(averaged) / logDenominator : 0.0f;
    const auto byteValue = static_cast<std::uint8_t>(std::clamp<std::int32_t>(
        static_cast<std::int32_t>(std::lround(normalized * 255.0f)), 0, 255));
    visData_.spectrum[0][i] = byteValue;
    visData_.spectrum[1][i] = byteValue;
  }
}
