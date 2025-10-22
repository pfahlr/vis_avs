#include "effects/render/effect_svp_loader.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <limits>
#include <mutex>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "audio/analyzer.h"
#include "avs/core/ParamBlock.hpp"
#include "avs/core/RenderContext.hpp"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace avs::effects::render {

namespace {

constexpr std::size_t kWaveformSize = 512u;
constexpr std::size_t kSpectrumSize = 256u;

using WaveformBuffer = std::array<std::array<std::uint8_t, kWaveformSize>, 2u>;
using SpectrumBuffer = std::array<std::array<std::uint8_t, kSpectrumSize>, 2u>;

struct SvpVisData {
  unsigned long millisecond = 0;
  WaveformBuffer waveform{};
  SpectrumBuffer spectrum{};
};

inline std::uint8_t clampByte(float value) {
  value = std::clamp(value, 0.0f, 255.0f);
  return static_cast<std::uint8_t>(value + 0.5f);
}

inline std::uint8_t waveformToByte(float sample) {
  constexpr float kScale = 127.5f;
  return clampByte((sample + 1.0f) * kScale);
}

inline std::uint8_t spectrumToByte(float magnitude) {
  return clampByte(magnitude * 255.0f);
}

double toMilliseconds(double seconds) {
  constexpr double kMaxMilliseconds = static_cast<double>(std::numeric_limits<unsigned long>::max());
  return std::clamp(seconds * 1000.0, 0.0, kMaxMilliseconds);
}

WaveformBuffer makeEmptyWaveform() {
  WaveformBuffer buffer{};
  for (auto& channel : buffer) {
    channel.fill(128u);
  }
  return buffer;
}

SpectrumBuffer makeEmptySpectrum() {
  SpectrumBuffer buffer{};
  for (auto& channel : buffer) {
    channel.fill(0u);
  }
  return buffer;
}

}  // namespace

class SvpLoader::Impl {
 public:
  virtual ~Impl() = default;
  virtual void setParams(const avs::core::ParamBlock& params) = 0;
  virtual bool render(avs::core::RenderContext& context) = 0;
};

namespace {

class StubImpl final : public SvpLoader::Impl {
 public:
  void setParams(const avs::core::ParamBlock& params) override {
    (void)params;
  }

  bool render(avs::core::RenderContext& context) override {
    (void)context;
    std::call_once(logOnce_, []() {
      std::clog << "Render / SVP Loader: platform does not support SVP plugins; effect is disabled."
                << std::endl;
    });
    return true;
  }

 private:
  std::once_flag logOnce_;
};

#if defined(_WIN32)

  class Win32Impl final : public SvpLoader::Impl {
 public:
  Win32Impl() = default;
  ~Win32Impl() override { unloadModule(); }

  void setParams(const avs::core::ParamBlock& params) override {
    std::lock_guard<std::mutex> guard(mutex_);
    const std::vector<std::string> keys = {"library", "file", "path", "module"};
    std::string value;
    for (const auto& key : keys) {
      if (params.contains(key)) {
        value = params.getString(key, "");
        if (!value.empty()) {
          break;
        }
      }
    }
    if (value != library_) {
      library_ = std::move(value);
      reloadPending_ = true;
    }
  }

  bool render(avs::core::RenderContext& context) override {
    std::lock_guard<std::mutex> guard(mutex_);
    if (reloadPending_) {
      loadModule();
      reloadPending_ = false;
    }

    if (!visInfo_ || !visInfo_->Render) {
      warnMissingModule();
      return true;
    }

    if (context.framebuffer.data == nullptr || context.width <= 0 || context.height <= 0) {
      return true;
    }

    const std::size_t requiredBytes =
        static_cast<std::size_t>(context.width) * static_cast<std::size_t>(context.height) * 4u;
    if (context.framebuffer.size < requiredBytes) {
      return true;
    }

    updateVisData(context);
    auto* pixels = reinterpret_cast<unsigned long*>(context.framebuffer.data);
    if (visInfo_->Render(pixels, context.width, context.height, context.width, &visData_) == 0) {
      warnRenderFailed();
    }
    return true;
  }

 private:
  using InitializeFn = void(__cdecl*)();
  using RenderFn = int(__cdecl*)(unsigned long*, int, int, int, SvpVisData*);
  using SettingsFn = int(__cdecl*)(char*);

  struct SvpVisInfo {
    unsigned long reserved = 0;
    const char* pluginName = nullptr;
    long required = 0;
    InitializeFn Initialize = nullptr;
    RenderFn Render = nullptr;
    SettingsFn SaveSettings = nullptr;
    SettingsFn OpenSettings = nullptr;
  };
  using QueryModuleFn = SvpVisInfo*(__cdecl*)();

  void loadModule() {
    unloadModule();
    if (library_.empty()) {
      return;
    }

    const auto resolved = resolveLibraryPath(library_);
    if (resolved.empty()) {
      warnMissingModule();
      return;
    }

    module_ = LoadLibraryW(resolved.c_str());
    if (!module_) {
      warnLoadFailure(resolved, GetLastError());
      return;
    }

    QueryModuleFn query = reinterpret_cast<QueryModuleFn>(GetProcAddress(module_, "QueryModule"));
    if (!query) {
      query = reinterpret_cast<QueryModuleFn>(
          GetProcAddress(module_, "?QueryModule@@YAPAUVisInfo@@XZ"));
    }
    if (!query) {
      warnQueryFailure();
      unloadModule();
      return;
    }

    visInfo_ = query();
    if (!visInfo_ || !visInfo_->Render) {
      warnInvalidModule();
      unloadModule();
      return;
    }

    settingsPath_ = resolved;
    settingsPath_.replace_filename(L"avs.ini");
    if (visInfo_->OpenSettings) {
      invokeSettings(visInfo_->OpenSettings);
    }
    if (visInfo_->Initialize) {
      visInfo_->Initialize();
    }
  }

  void unloadModule() {
    if (visInfo_ && visInfo_->SaveSettings && !settingsPath_.empty()) {
      invokeSettings(visInfo_->SaveSettings);
    }
    visInfo_ = nullptr;
    if (module_) {
      FreeLibrary(module_);
      module_ = nullptr;
    }
  }

  void updateVisData(const avs::core::RenderContext& context) {
    accumulatedSeconds_ += std::max(context.deltaSeconds, 0.0);
    visData_.millisecond = static_cast<unsigned long>(toMilliseconds(accumulatedSeconds_));

    if (const auto* analysis = context.audioAnalysis) {
      fillFromAnalysis(*analysis);
    } else if (context.audioSpectrum.data &&
               context.audioSpectrum.size >= avs::audio::Analysis::kSpectrumSize) {
      fillFromSpectrumView(context);
    } else {
      visData_.waveform = makeEmptyWaveform();
      visData_.spectrum = makeEmptySpectrum();
    }
  }

  void fillFromAnalysis(const avs::audio::Analysis& analysis) {
    const auto& waveform = analysis.waveform;
    const auto& spectrum = analysis.spectrum;
    for (std::size_t i = 0; i < kWaveformSize; ++i) {
      const double srcPos =
          (static_cast<double>(i) *
           static_cast<double>(avs::audio::Analysis::kWaveformSize - 1u)) /
          static_cast<double>(kWaveformSize - 1u);
      const auto index = static_cast<std::size_t>(
          std::clamp(srcPos, 0.0,
                     static_cast<double>(avs::audio::Analysis::kWaveformSize - 1u)));
      const float sample = waveform[index];
      const std::uint8_t value = waveformToByte(sample);
      visData_.waveform[0][i] = value;
      visData_.waveform[1][i] = value;
    }

    for (std::size_t i = 0; i < kSpectrumSize; ++i) {
      const std::size_t base = std::min(i * 2u, spectrum.size() - 1u);
      const float a = spectrum[base];
      const float b = spectrum[std::min(base + 1u, spectrum.size() - 1u)];
      const float average = (a + b) * 0.5f;
      const std::uint8_t value = spectrumToByte(average);
      visData_.spectrum[0][i] = value;
      visData_.spectrum[1][i] = value;
    }
  }

  void fillFromSpectrumView(const avs::core::RenderContext& context) {
    const float* spectrum = context.audioSpectrum.data;
    const std::size_t available = context.audioSpectrum.size;
    visData_.waveform = makeEmptyWaveform();
    for (std::size_t i = 0; i < kSpectrumSize; ++i) {
      const std::size_t base = std::min(i * 2u, available - 1u);
      const float a = spectrum[base];
      const float b = spectrum[std::min(base + 1u, available - 1u)];
      const std::uint8_t value = spectrumToByte((a + b) * 0.5f);
      visData_.spectrum[0][i] = value;
      visData_.spectrum[1][i] = value;
    }
  }

  void warnMissingModule() {
    if (!library_.empty()) {
      std::call_once(missingModuleOnce_, [this]() {
        std::clog << "Render / SVP Loader: unable to load module '" << library_
                  << "'." << std::endl;
      });
    }
  }

  void warnLoadFailure(const std::wstring& path, DWORD error) {
    std::call_once(loadFailureOnce_, [&]() {
      const std::string narrow = std::filesystem::path(path).string();
      std::clog << "Render / SVP Loader: LoadLibrary failed for '" << narrow
                << "' (error " << error << ")." << std::endl;
    });
  }

  void warnQueryFailure() {
    std::call_once(queryFailureOnce_, []() {
      std::clog << "Render / SVP Loader: module does not export QueryModule." << std::endl;
    });
  }

  void warnInvalidModule() {
    std::call_once(invalidModuleOnce_, []() {
      std::clog << "Render / SVP Loader: module did not provide a valid VisInfo." << std::endl;
    });
  }

  void warnRenderFailed() {
    std::call_once(renderFailedOnce_, []() {
      std::clog << "Render / SVP Loader: plugin render call returned failure." << std::endl;
    });
  }

  std::wstring resolveLibraryPath(const std::string& library) {
    namespace fs = std::filesystem;
    const fs::path candidateUtf8{library};
    std::vector<fs::path> candidates;
    if (candidateUtf8.is_absolute()) {
      candidates.push_back(candidateUtf8);
    } else {
      std::error_code ec;
      const fs::path current = fs::current_path(ec);
      if (!ec) {
        candidates.push_back(current / candidateUtf8);
        if (!library.empty()) {
          candidates.push_back(current / "plugins" / candidateUtf8);
        }
      }
      if (!library.empty()) {
        candidates.emplace_back(candidateUtf8);
      }
    }

    std::error_code ec;
    for (const auto& candidate : candidates) {
      if (fs::exists(candidate, ec) && !ec) {
        return candidate.wstring();
      }
      ec.clear();
    }
    return {};
  }

  void invokeSettings(SettingsFn fn) {
    if (!fn || settingsPath_.empty()) {
      return;
    }
    std::string ansiPath = std::filesystem::path(settingsPath_).string();
    std::vector<char> buffer(ansiPath.begin(), ansiPath.end());
    buffer.push_back('\0');
    fn(buffer.data());
  }

  std::mutex mutex_;
  std::string library_;
  bool reloadPending_ = false;

  HMODULE module_ = nullptr;
  SvpVisInfo* visInfo_ = nullptr;
  std::wstring settingsPath_;
  double accumulatedSeconds_ = 0.0;
  SvpVisData visData_{};

  std::once_flag missingModuleOnce_;
  std::once_flag loadFailureOnce_;
  std::once_flag queryFailureOnce_;
  std::once_flag invalidModuleOnce_;
  std::once_flag renderFailedOnce_;
};

#endif  // defined(_WIN32)

}  // namespace

SvpLoader::SvpLoader()
    : impl_(
#if defined(_WIN32)
          std::make_unique<Win32Impl>()
#else
          std::make_unique<StubImpl>()
#endif
      ) {}

SvpLoader::~SvpLoader() = default;

bool SvpLoader::render(avs::core::RenderContext& context) { return impl_->render(context); }

void SvpLoader::setParams(const avs::core::ParamBlock& params) { impl_->setParams(params); }

}  // namespace avs::effects::render

