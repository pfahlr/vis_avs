#include "effects/render/effect_svp_loader.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "audio/analyzer.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif  // WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif  // _WIN32

namespace avs::effects::render {
namespace {

constexpr int kWaveformSamples = 512;
constexpr int kSpectrumSamples = 256;
constexpr std::size_t kChannels = 2;

struct SvpVisData {
  unsigned long MillSec = 0;
  unsigned char Waveform[kChannels][kWaveformSamples]{};
  unsigned char Spectrum[kChannels][kSpectrumSamples]{};
};

float clampFinite(float value, float minValue, float maxValue) {
  if (!std::isfinite(value)) {
    return 0.0f;
  }
  return std::clamp(value, minValue, maxValue);
}

#ifdef _WIN32
struct SvpVisInfo {
  unsigned long reserved;
  char* pluginName;
  long required;
  void(__cdecl* Initialize)();
  int(__cdecl* Render)(unsigned long*, int, int, int, SvpVisData*);
  int(__cdecl* SaveSettings)(char*);
  int(__cdecl* OpenSettings)(char*);
};

using QueryModuleProc = SvpVisInfo*(__cdecl*)();
#endif  // _WIN32

std::string stripWhitespace(std::string value) {
  value.erase(
      std::remove_if(value.begin(), value.end(),
                     [](unsigned char ch) { return ch == '\r' || ch == '\n' || ch == '\t'; }),
      value.end());
  return value;
}

std::string extractLibraryPath(const avs::core::ParamBlock& params) {
  static constexpr std::array<std::string_view, 6> kKeys = {"library", "file",   "filename",
                                                            "path",    "plugin", "svp"};
  for (const auto& keyView : kKeys) {
    const std::string key(keyView);
    if (!params.contains(key)) {
      continue;
    }
    auto value = params.getString(key, "");
    value = stripWhitespace(value);
    if (!value.empty()) {
      return value;
    }
  }
  return {};
}

unsigned char waveformSampleToByte(float sample) {
  const float clamped = clampFinite(sample, -1.0f, 1.0f);
  const int scaled = static_cast<int>(std::lround(clamped * 127.0f + 128.0f));
  return static_cast<unsigned char>(std::clamp(scaled, 0, 255));
}

unsigned char spectrumSampleToByte(float sample) {
  const float clamped = clampFinite(sample, 0.0f, 1.0f);
  const int scaled = static_cast<int>(std::lround(clamped * 255.0f));
  return static_cast<unsigned char>(std::clamp(scaled, 0, 255));
}

}  // namespace

class SvpLoader::Impl {
 public:
  Impl() = default;
  ~Impl() { unloadLibrary(); }

  void setParams(const avs::core::ParamBlock& params) {
    const std::string candidate = extractLibraryPath(params);
    std::lock_guard<std::mutex> lock(mutex_);
    if (candidate != libraryPath_) {
      libraryPath_ = candidate;
      libraryDirty_ = true;
      reportedMissingLibrary_ = false;
      reportedUnsupported_ = false;
    }
  }

  bool render(avs::core::RenderContext& context) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!hasFramebuffer(context)) {
      return true;
    }

    updateClock();
    updateVisData(context);

#ifdef _WIN32
    if (!ensureLibrary()) {
      return true;
    }
    if (!visInfo_ || !visInfo_->Render) {
      return true;
    }
    if (!prepareSurface(context)) {
      return true;
    }
    visInfo_->Render(surfaceBuffer_.data(), context.width, context.height, context.width,
                     &visData_);
    commitSurface(context);
#else
    if (!libraryPath_.empty() && !reportedUnsupported_) {
      std::clog << "SVP Loader: platform does not support SVP modules, effect disabled for '"
                << libraryPath_ << "'\n";
      reportedUnsupported_ = true;
    }
    libraryDirty_ = false;
#endif
    return true;
  }

 private:
  [[nodiscard]] bool hasFramebuffer(const avs::core::RenderContext& context) const {
    if (!context.framebuffer.data) {
      return false;
    }
    if (context.width <= 0 || context.height <= 0) {
      return false;
    }
    const std::size_t requiredBytes =
        static_cast<std::size_t>(context.width) * static_cast<std::size_t>(context.height) * 4u;
    return context.framebuffer.size >= requiredBytes;
  }

  void updateClock() {
    const auto now = std::chrono::steady_clock::now();
    if (!clockStarted_) {
      startTime_ = now;
      clockStarted_ = true;
    }
    const auto millis =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime_).count();
    visData_.MillSec = static_cast<unsigned long>(std::max<std::int64_t>(millis, 0));
  }

  void updateVisData(const avs::core::RenderContext& context) {
    const avs::audio::Analysis* analysis = context.audioAnalysis;
    if (!analysis) {
      clearVisData();
      return;
    }

    const std::size_t waveformSize = analysis->waveform.size();
    if (waveformSize > 0) {
      const double scale =
          static_cast<double>(waveformSize) / static_cast<double>(kWaveformSamples);
      for (std::size_t channel = 0; channel < kChannels; ++channel) {
        for (int i = 0; i < kWaveformSamples; ++i) {
          const double start = static_cast<double>(i) * scale;
          double end = start + scale;
          std::size_t beginIndex = static_cast<std::size_t>(std::floor(start));
          std::size_t endIndex = static_cast<std::size_t>(std::floor(end));
          if (endIndex <= beginIndex) {
            endIndex = std::min(beginIndex + 1, waveformSize);
          }
          float sum = 0.0f;
          std::size_t count = 0;
          for (std::size_t j = beginIndex; j < endIndex && j < waveformSize; ++j) {
            sum += clampFinite(analysis->waveform[j], -1.0f, 1.0f);
            ++count;
          }
          const float value = count > 0 ? sum / static_cast<float>(count) : 0.0f;
          visData_.Waveform[channel][i] = waveformSampleToByte(value);
        }
      }
    } else {
      for (auto& channel : visData_.Waveform) {
        std::fill(std::begin(channel), std::end(channel), 128u);
      }
    }

    const std::size_t spectrumSize = analysis->spectrum.size();
    if (spectrumSize > 0) {
      for (int i = 0; i < kSpectrumSamples; ++i) {
        const std::size_t index0 = std::min<std::size_t>(i * 2, spectrumSize - 1);
        const std::size_t index1 = std::min<std::size_t>(index0 + 1, spectrumSize - 1);
        const float value = (clampFinite(analysis->spectrum[index0], 0.0f, 1.0f) +
                             clampFinite(analysis->spectrum[index1], 0.0f, 1.0f)) *
                            0.5f;
        const unsigned char sample = spectrumSampleToByte(value);
        visData_.Spectrum[0][i] = sample;
        visData_.Spectrum[1][i] = sample;
      }
    } else {
      for (auto& channel : visData_.Spectrum) {
        std::fill(std::begin(channel), std::end(channel), 0u);
      }
    }
  }

  void clearVisData() { visData_ = SvpVisData{}; }

#ifdef _WIN32
  bool ensureLibrary() {
    if (!libraryDirty_ && visInfo_) {
      return true;
    }
    if (libraryPath_.empty()) {
      if (!reportedMissingLibrary_) {
        std::clog << "SVP Loader: no library configured" << std::endl;
        reportedMissingLibrary_ = true;
      }
      libraryDirty_ = false;
      return false;
    }

    unloadLibrary();
    libraryDirty_ = false;

    const auto resolved = resolveLibraryPath(libraryPath_);
    if (!resolved) {
      if (!reportedMissingLibrary_) {
        std::clog << "SVP Loader: unable to locate '" << libraryPath_ << "'" << std::endl;
        reportedMissingLibrary_ = true;
      }
      return false;
    }

    resolvedLibraryPath_ = *resolved;

    moduleHandle_ = ::LoadLibraryW(resolved->c_str());
    if (!moduleHandle_) {
      std::clog << "SVP Loader: failed to load '" << resolvedLibraryPath_.string() << "'"
                << std::endl;
      return false;
    }

    QueryModuleProc query =
        reinterpret_cast<QueryModuleProc>(::GetProcAddress(moduleHandle_, "QueryModule"));
    if (!query) {
      query = reinterpret_cast<QueryModuleProc>(
          ::GetProcAddress(moduleHandle_, "?QueryModule@@YAPAUVisInfo@@XZ"));
    }
    if (!query) {
      std::clog << "SVP Loader: QueryModule entry point not found in '"
                << resolvedLibraryPath_.string() << "'" << std::endl;
      unloadLibrary();
      return false;
    }

    visInfo_ = query();
    if (!visInfo_) {
      std::clog << "SVP Loader: QueryModule returned null for '" << resolvedLibraryPath_.string()
                << "'" << std::endl;
      unloadLibrary();
      return false;
    }

    if (visInfo_->Initialize) {
      visInfo_->Initialize();
    }

    reportedMissingLibrary_ = false;
    return true;
  }

  void unloadLibrary() {
    visInfo_ = nullptr;
    if (moduleHandle_) {
      ::FreeLibrary(moduleHandle_);
      moduleHandle_ = nullptr;
    }
  }

  static std::optional<std::filesystem::path> resolveLibraryPath(const std::string& rawPath) {
    namespace fs = std::filesystem;
    std::vector<fs::path> attempts;
    fs::path base(rawPath);
    attempts.push_back(base);
    if (!base.has_extension()) {
      attempts.push_back(base.string() + ".svp");
      attempts.push_back(base.string() + ".SVP");
      attempts.push_back(base.string() + ".uvs");
      attempts.push_back(base.string() + ".UVS");
      attempts.push_back(base.string() + ".dll");
      attempts.push_back(base.string() + ".DLL");
    }

    std::error_code ec;
    for (auto candidate : attempts) {
      if (candidate.empty()) {
        continue;
      }
      fs::path absolute = candidate.is_absolute() ? candidate : fs::absolute(candidate, ec);
      if (ec) {
        ec.clear();
        continue;
      }
      if (fs::exists(absolute, ec) && !ec) {
        return absolute;
      }
      ec.clear();
    }
    return std::nullopt;
  }

  bool prepareSurface(const avs::core::RenderContext& context) {
    const std::size_t pixelCount =
        static_cast<std::size_t>(context.width) * static_cast<std::size_t>(context.height);
    surfaceBuffer_.resize(pixelCount);
    const std::uint8_t* src = context.framebuffer.data;
    for (std::size_t i = 0; i < pixelCount; ++i) {
      const std::size_t offset = i * 4u;
      surfaceBuffer_[i] = (static_cast<std::uint32_t>(src[offset + 3]) << 24u) |
                          (static_cast<std::uint32_t>(src[offset + 0]) << 16u) |
                          (static_cast<std::uint32_t>(src[offset + 1]) << 8u) |
                          static_cast<std::uint32_t>(src[offset + 2]);
    }
    return true;
  }

  void commitSurface(avs::core::RenderContext& context) {
    std::uint8_t* dst = context.framebuffer.data;
    for (std::size_t i = 0; i < surfaceBuffer_.size(); ++i) {
      const std::uint32_t pixel = surfaceBuffer_[i];
      const std::size_t offset = i * 4u;
      dst[offset + 0] = static_cast<std::uint8_t>((pixel >> 16u) & 0xFFu);
      dst[offset + 1] = static_cast<std::uint8_t>((pixel >> 8u) & 0xFFu);
      dst[offset + 2] = static_cast<std::uint8_t>(pixel & 0xFFu);
      dst[offset + 3] = static_cast<std::uint8_t>((pixel >> 24u) & 0xFFu);
    }
  }

  HMODULE moduleHandle_ = nullptr;
  SvpVisInfo* visInfo_ = nullptr;
#else
  void unloadLibrary() {}
#endif  // _WIN32

  std::string libraryPath_;
  bool libraryDirty_ = false;
  bool reportedMissingLibrary_ = false;
  bool reportedUnsupported_ = false;
  SvpVisData visData_{};
  std::vector<std::uint32_t> surfaceBuffer_;
  std::mutex mutex_;
  std::chrono::steady_clock::time_point startTime_{};
  bool clockStarted_ = false;
#ifdef _WIN32
  std::filesystem::path resolvedLibraryPath_;
#endif
};

SvpLoader::SvpLoader() : impl_(std::make_unique<Impl>()) {}
SvpLoader::~SvpLoader() = default;

void SvpLoader::setParams(const avs::core::ParamBlock& params) { impl_->setParams(params); }

bool SvpLoader::render(avs::core::RenderContext& context) { return impl_->render(context); }

}  // namespace avs::effects::render
