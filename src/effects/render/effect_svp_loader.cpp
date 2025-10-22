#include "effects/render/effect_svp_loader.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>

#include "audio/analyzer.h"

#if defined(_WIN32)
#  include <filesystem>
#  include <system_error>
#endif

namespace avs::effects::render {

namespace {

#if defined(_WIN32)
std::filesystem::path resolveLibraryPath(const std::string& value) {
  if (value.empty()) {
    return {};
  }
  std::filesystem::path candidate = std::filesystem::u8path(value);
  if (candidate.is_absolute()) {
    return candidate;
  }
  std::error_code ec;
  std::filesystem::path cwd = std::filesystem::current_path(ec);
  if (ec) {
    return candidate;
  }
  return cwd / candidate;
}

std::uint32_t toAbgr(std::uint32_t rgba) {
  const std::uint32_t r = (rgba)&0xFFu;
  const std::uint32_t g = (rgba >> 8) & 0xFFu;
  const std::uint32_t b = (rgba >> 16) & 0xFFu;
  const std::uint32_t a = (rgba >> 24) & 0xFFu;
  return (a << 24) | (b << 16) | (g << 8) | r;
}

std::uint32_t toRgba(std::uint32_t abgr) {
  const std::uint32_t a = (abgr >> 24) & 0xFFu;
  const std::uint32_t b = (abgr >> 16) & 0xFFu;
  const std::uint32_t g = (abgr >> 8) & 0xFFu;
  const std::uint32_t r = abgr & 0xFFu;
  return (r) | (g << 8) | (b << 16) | (a << 24);
}
#endif

std::string extractLibraryParam(const avs::core::ParamBlock& params,
                                const std::string& current) {
  static constexpr std::array<std::string_view, 4> kKeys{"library", "file", "path", "svp"};
  for (std::string_view keyView : kKeys) {
    const std::string key{keyView};
    if (params.contains(key)) {
      return params.getString(key, current);
    }
  }
  return current;
}

}  // namespace

SvpLoader::SvpLoader() = default;

SvpLoader::~SvpLoader() {
#if defined(_WIN32)
  unloadPlugin();
#endif
}

void SvpLoader::setParams(const avs::core::ParamBlock& params) {
  const std::string requested = extractLibraryParam(params, library_);
  if (requested != library_) {
    library_ = requested;
    reloadPending_ = true;
    fallbackNotified_ = false;
  }
}

bool SvpLoader::render(avs::core::RenderContext& context) {
#if defined(_WIN32)
  if (!ensurePluginLoaded()) {
    renderFallback(context);
    return true;
  }

  updateAudioData(context);

  if (!visInfo_ || !visInfo_->Render) {
    renderFallback(context);
    return true;
  }

  if (context.width <= 0 || context.height <= 0 || context.framebuffer.data == nullptr) {
    return true;
  }

  const std::size_t pixelCount = static_cast<std::size_t>(context.width) *
                                 static_cast<std::size_t>(context.height);
  if (pixelCount == 0 || context.framebuffer.size < pixelCount * 4) {
    return true;
  }

  auto* pixels = reinterpret_cast<std::uint32_t*>(context.framebuffer.data);
  convertBufferToAbgr(pixels, pixelCount);
  visInfo_->Render(pixels, context.width, context.height, context.width, &visData_);
  convertBufferToRgba(pixels, pixelCount);
  return true;
#else
  (void)context;
  renderFallback(context);
  return true;
#endif
}

void SvpLoader::renderFallback(avs::core::RenderContext& context) {
  const std::size_t pixelCount = static_cast<std::size_t>(std::max(context.width, 0)) *
                                 static_cast<std::size_t>(std::max(context.height, 0));
  if (context.framebuffer.data != nullptr && context.framebuffer.size >= pixelCount * 4) {
    std::fill(context.framebuffer.data, context.framebuffer.data + pixelCount * 4, 0u);
  }
  if (!fallbackNotified_) {
    std::cerr << "SVP loader fallback active";
    if (!library_.empty()) {
      std::cerr << ": unable to load '" << library_ << "'";
    }
#if defined(_WIN32)
    if (!loadedPath_.empty()) {
      std::cerr << " (resolved path: '" << loadedPath_.u8string() << "')";
    }
#endif
    std::cerr << std::endl;
    fallbackNotified_ = true;
  }
}

#if defined(_WIN32)

bool SvpLoader::ensurePluginLoaded() {
  if (!reloadPending_ && module_ != nullptr && visInfo_ != nullptr) {
    return true;
  }

  unloadPlugin();
  if (library_.empty()) {
    reloadPending_ = false;
    return false;
  }

  const std::filesystem::path candidate = resolveLibraryPath(library_);
  if (!loadPlugin(candidate)) {
    reloadPending_ = false;
    return false;
  }

  reloadPending_ = false;
  fallbackNotified_ = false;
  return true;
}

bool SvpLoader::loadPlugin(const std::filesystem::path& candidatePath) {
  loadedPath_.clear();
  std::error_code existsError;
  if (!std::filesystem::exists(candidatePath, existsError)) {
    return false;
  }

  const std::wstring widePath = candidatePath.wstring();
  HMODULE module = LoadLibraryW(widePath.c_str());
  if (!module) {
    return false;
  }

  using QueryModuleFunc = VisInfo*(__cdecl*)();
  auto* queryModule = reinterpret_cast<QueryModuleFunc>(GetProcAddress(module, "QueryModule"));
  if (!queryModule) {
    queryModule = reinterpret_cast<QueryModuleFunc>(
        GetProcAddress(module, "?QueryModule@@YAPAUUltraVisInfo@@XZ"));
  }

  if (!queryModule) {
    FreeLibrary(module);
    return false;
  }

  VisInfo* info = queryModule();
  if (!info || !info->Render) {
    FreeLibrary(module);
    return false;
  }

  module_ = module;
  visInfo_ = info;
  loadedPath_ = candidatePath;

  if (visInfo_->OpenSettings) {
    char iniFile[] = "avs.ini";
    visInfo_->OpenSettings(iniFile);
  }
  if (visInfo_->Initialize) {
    visInfo_->Initialize();
  }

  return true;
}

void SvpLoader::unloadPlugin() {
  if (visInfo_ && visInfo_->SaveSettings) {
    char iniFile[] = "avs.ini";
    visInfo_->SaveSettings(iniFile);
  }
  if (module_) {
    FreeLibrary(module_);
  }
  module_ = nullptr;
  visInfo_ = nullptr;
  loadedPath_.clear();
}

void SvpLoader::updateAudioData(const avs::core::RenderContext& context) {
  const auto now = std::chrono::steady_clock::now();
  const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
  visData_.MillSec = static_cast<unsigned long>(nowMs.count() & 0xFFFFFFFFu);

  const avs::audio::Analysis* analysis = context.audioAnalysis;
  if (!analysis) {
    std::memset(visData_.Waveform, 0, sizeof(visData_.Waveform));
    std::memset(visData_.Spectrum, 0, sizeof(visData_.Spectrum));
    return;
  }

  const auto sampleCount = analysis->waveform.size();
  if (sampleCount > 0) {
    for (std::size_t i = 0; i < 512; ++i) {
      const float position = static_cast<float>(i) * static_cast<float>(sampleCount - 1) / 511.0f;
      const std::size_t index = static_cast<std::size_t>(position);
      const std::size_t next = std::min(index + 1, sampleCount - 1);
      const float frac = position - static_cast<float>(index);
      const float sample = analysis->waveform[index] * (1.0f - frac) +
                           analysis->waveform[next] * frac;
      const float normalized = std::clamp((sample + 1.0f) * 127.5f, 0.0f, 255.0f);
      const auto byteValue = static_cast<unsigned char>(normalized);
      visData_.Waveform[0][i] = byteValue;
      visData_.Waveform[1][i] = byteValue;
    }
  } else {
    std::memset(visData_.Waveform, 0, sizeof(visData_.Waveform));
  }

  const auto binCount = analysis->spectrum.size();
  if (binCount > 0) {
    for (std::size_t i = 0; i < 256; ++i) {
      const float position = static_cast<float>(i) * static_cast<float>(binCount - 1) / 255.0f;
      const std::size_t index = static_cast<std::size_t>(position);
      const std::size_t next = std::min(index + 1, binCount - 1);
      const float frac = position - static_cast<float>(index);
      const float sample = analysis->spectrum[index] * (1.0f - frac) +
                           analysis->spectrum[next] * frac;
      const float normalized = std::clamp(sample * 255.0f, 0.0f, 255.0f);
      const auto byteValue = static_cast<unsigned char>(normalized);
      visData_.Spectrum[0][i] = byteValue;
      visData_.Spectrum[1][i] = byteValue;
    }
  } else {
    std::memset(visData_.Spectrum, 0, sizeof(visData_.Spectrum));
  }
}

void SvpLoader::convertBufferToAbgr(std::uint32_t* pixels, std::size_t count) {
  if (!pixels) {
    return;
  }
  for (std::size_t i = 0; i < count; ++i) {
    pixels[i] = toAbgr(pixels[i]);
  }
}

void SvpLoader::convertBufferToRgba(std::uint32_t* pixels, std::size_t count) {
  if (!pixels) {
    return;
  }
  for (std::size_t i = 0; i < count; ++i) {
    pixels[i] = toRgba(pixels[i]);
  }
}

#endif  // defined(_WIN32)

}  // namespace avs::effects::render
