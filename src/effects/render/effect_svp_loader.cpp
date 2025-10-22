#include "effects/render/effect_svp_loader.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "audio/analyzer.h"

#if defined(_WIN32)
#include <windows.h>

#include <filesystem>
#include <mutex>
#include <type_traits>
#endif

namespace {

std::string trimWhitespace(std::string_view value) {
  const auto begin = value.find_first_not_of(" \t\n\r\f\v");
  if (begin == std::string_view::npos) {
    return {};
  }
  const auto end = value.find_last_not_of(" \t\n\r\f\v");
  return std::string(value.substr(begin, end - begin + 1u));
}

std::string readLibraryParam(const avs::core::ParamBlock& params) {
  const std::array<std::string_view, 6> keys{std::string_view{"library"}, std::string_view{"file"},
                                             std::string_view{"path"},    std::string_view{"svp"},
                                             std::string_view{"module"},  std::string_view{"dll"}};
  for (const auto& keyView : keys) {
    const std::string key{keyView};
    const std::string value = params.getString(key, "");
    const std::string trimmed = trimWhitespace(value);
    if (!trimmed.empty()) {
      return trimmed;
    }
  }
  return {};
}

#if defined(_WIN32)

struct VisData {
  unsigned long MillSec = 0;
  std::array<std::array<std::uint8_t, 512>, 2> Waveform{};
  std::array<std::array<std::uint8_t, 256>, 2> Spectrum{};
};

struct VisInfo {
  unsigned long Reserved = 0;
  char* PluginName = nullptr;
  long lRequired = 0;
  void (*Initialize)(void) = nullptr;
  BOOL (*Render)(unsigned long* Video, int width, int height, int pitch, VisData* data) = nullptr;
  BOOL (*SaveSettings)(char* fileName) = nullptr;
  BOOL (*OpenSettings)(char* fileName) = nullptr;
};

using QueryModuleFn = VisInfo*(__cdecl*)();

constexpr long kRequireWaveform = 0x0001;
constexpr long kRequireSpectrum = 0x0002;

void swapRedBlue(std::uint8_t* pixel) { std::swap(pixel[0], pixel[2]); }

#endif

}  // namespace

namespace avs::effects::render {

struct SvpLoader::Impl {
  Impl() = default;
  ~Impl();

  void setParams(const avs::core::ParamBlock& params);
  bool render(avs::core::RenderContext& context);

  std::string requestedLibrary_;

#if defined(_WIN32)
  bool ensureLibraryLoadedLocked();
  void unloadLocked();
  static std::wstring utf8ToWide(const std::string& value);
  static void convertAbgrToRgba(std::uint8_t* data, int width, int height);
  static float sampleWaveform(const avs::audio::Analysis& analysis, std::size_t index);
  static float sampleSpectrum(const avs::audio::Analysis& analysis, std::size_t band);
  static unsigned long toMilliseconds(std::uint64_t frameIndex, double deltaSeconds);
  static VisData makeVisData(const avs::core::RenderContext& context);
  std::filesystem::path resolveLibraryPath(const std::string& name) const;
  std::string settingsFileFor(const std::filesystem::path& library) const;

  mutable std::mutex mutex_;
  std::filesystem::path resolvedLibrary_;
  bool attemptedLoad_ = false;
  std::string settingsPath_;
  struct LibraryHandleDeleter {
    void operator()(HMODULE module) const noexcept {
      if (module) {
        FreeLibrary(module);
      }
    }
  };
  using LibraryHandle = std::unique_ptr<std::remove_pointer_t<HMODULE>, LibraryHandleDeleter>;
  LibraryHandle library_{};
  VisInfo* visInfo_ = nullptr;
#else
  bool warnedUnsupported_ = false;
#endif
};

SvpLoader::Impl::~Impl() {
#if defined(_WIN32)
  std::lock_guard<std::mutex> lock(mutex_);
  unloadLocked();
#endif
}

void SvpLoader::Impl::setParams(const avs::core::ParamBlock& params) {
  const std::string nextLibrary = readLibraryParam(params);
#if defined(_WIN32)
  std::lock_guard<std::mutex> lock(mutex_);
  if (requestedLibrary_ == nextLibrary) {
    return;
  }
  requestedLibrary_ = nextLibrary;
  attemptedLoad_ = false;
  unloadLocked();
#else
  requestedLibrary_ = nextLibrary;
  warnedUnsupported_ = false;
#endif
}

bool SvpLoader::Impl::render(avs::core::RenderContext& context) {
#if defined(_WIN32)
  std::lock_guard<std::mutex> lock(mutex_);
  if (!attemptedLoad_) {
    attemptedLoad_ = true;
    if (!ensureLibraryLoadedLocked()) {
      return true;
    }
  }

  if (!visInfo_ || !library_) {
    return true;
  }
  if (!visInfo_->Render) {
    return true;
  }
  if (!context.framebuffer.data || context.width <= 0 || context.height <= 0) {
    return true;
  }
  const std::size_t requiredPixels =
      static_cast<std::size_t>(context.width) * static_cast<std::size_t>(context.height) * 4u;
  if (context.framebuffer.size < requiredPixels) {
    return true;
  }

  VisData data = makeVisData(context);
  const BOOL result = visInfo_->Render(reinterpret_cast<unsigned long*>(context.framebuffer.data),
                                       context.width, context.height, context.width, &data);
  convertAbgrToRgba(context.framebuffer.data, context.width, context.height);
  return result != FALSE;
#else
  (void)context;
  if (!warnedUnsupported_ && !requestedLibrary_.empty()) {
    warnedUnsupported_ = true;
  }
  return true;
#endif
}

#if defined(_WIN32)

bool SvpLoader::Impl::ensureLibraryLoadedLocked() {
  unloadLocked();
  if (requestedLibrary_.empty()) {
    return false;
  }

  const std::filesystem::path libraryPath = resolveLibraryPath(requestedLibrary_);
  if (libraryPath.empty()) {
    return false;
  }

  const std::wstring widePath = utf8ToWide(libraryPath.u8string());
  if (widePath.empty()) {
    return false;
  }

  HMODULE module = LoadLibraryW(widePath.c_str());
  if (!module) {
    return false;
  }

  LibraryHandle handle{module};

  QueryModuleFn query = reinterpret_cast<QueryModuleFn>(GetProcAddress(module, "QueryModule"));
  if (!query) {
    query = reinterpret_cast<QueryModuleFn>(
        GetProcAddress(module, "?QueryModule@@YAPAUUltraVisInfo@@XZ"));
  }
  if (!query) {
    query =
        reinterpret_cast<QueryModuleFn>(GetProcAddress(module, "?QueryModule@@YAPAUVisInfo@@XZ"));
  }
  if (!query) {
    return false;
  }

  VisInfo* info = query();
  if (!info) {
    return false;
  }

  library_ = std::move(handle);
  visInfo_ = info;
  resolvedLibrary_ = libraryPath;
  settingsPath_ = settingsFileFor(libraryPath);

  if (!settingsPath_.empty() && visInfo_->OpenSettings) {
    visInfo_->OpenSettings(settingsPath_.data());
  }
  if (visInfo_->Initialize) {
    visInfo_->Initialize();
  }
  return true;
}

void SvpLoader::Impl::unloadLocked() {
  if (visInfo_ && visInfo_->SaveSettings && !settingsPath_.empty()) {
    visInfo_->SaveSettings(settingsPath_.data());
  }
  visInfo_ = nullptr;
  library_.reset();
  resolvedLibrary_.clear();
  settingsPath_.clear();
}

std::wstring SvpLoader::Impl::utf8ToWide(const std::string& value) {
  if (value.empty()) {
    return {};
  }
  const int required = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, nullptr, 0);
  if (required <= 0) {
    return {};
  }
  std::wstring wide;
  wide.resize(static_cast<std::size_t>(required - 1));
  const int converted = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, wide.data(), required);
  if (converted <= 0) {
    return {};
  }
  return wide;
}

void SvpLoader::Impl::convertAbgrToRgba(std::uint8_t* data, int width, int height) {
  if (!data || width <= 0 || height <= 0) {
    return;
  }
  const std::size_t pixels = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
  for (std::size_t i = 0; i < pixels; ++i) {
    swapRedBlue(data + i * 4u);
  }
}

float SvpLoader::Impl::sampleWaveform(const avs::audio::Analysis& analysis, std::size_t index) {
  const double maxIndex = static_cast<double>(avs::audio::Analysis::kWaveformSize - 1u);
  const double position = maxIndex * static_cast<double>(index) / 511.0;
  const std::size_t base = static_cast<std::size_t>(std::floor(position));
  const double fraction = position - static_cast<double>(base);
  const std::size_t next = std::min(base + 1u, avs::audio::Analysis::kWaveformSize - 1u);
  const float a = analysis.waveform[base];
  const float b = analysis.waveform[next];
  return a + static_cast<float>(fraction) * (b - a);
}

float SvpLoader::Impl::sampleSpectrum(const avs::audio::Analysis& analysis, std::size_t band) {
  const double bins = static_cast<double>(avs::audio::Analysis::kSpectrumSize);
  const double begin = bins * static_cast<double>(band) / 256.0;
  const double end = bins * static_cast<double>(band + 1u) / 256.0;
  const std::size_t start = static_cast<std::size_t>(std::floor(begin));
  const std::size_t finish = static_cast<std::size_t>(std::ceil(end));
  float sum = 0.0f;
  int count = 0;
  for (std::size_t i = start; i < finish && i < avs::audio::Analysis::kSpectrumSize; ++i) {
    float magnitude = analysis.spectrum[i];
    if (!std::isfinite(magnitude) || magnitude < 0.0f) {
      magnitude = 0.0f;
    }
    sum += magnitude;
    ++count;
  }
  if (count == 0) {
    return 0.0f;
  }
  return sum / static_cast<float>(count);
}

unsigned long SvpLoader::Impl::toMilliseconds(std::uint64_t frameIndex, double deltaSeconds) {
  const double timeMs = static_cast<double>(frameIndex) * std::max(deltaSeconds, 0.0) * 1000.0;
  if (!std::isfinite(timeMs) || timeMs <= 0.0) {
    return 0u;
  }
  const double clamped =
      std::min(timeMs, static_cast<double>(std::numeric_limits<unsigned long>::max()));
  return static_cast<unsigned long>(clamped);
}

VisData SvpLoader::Impl::makeVisData(const avs::core::RenderContext& context) {
  VisData data{};
  data.MillSec = toMilliseconds(context.frameIndex, context.deltaSeconds);

  std::array<std::uint8_t, 512> waveform;
  waveform.fill(128u);
  std::array<std::uint8_t, 256> spectrum;
  spectrum.fill(0u);

  const avs::audio::Analysis* analysis = context.audioAnalysis;
  if (analysis) {
    if ((visInfo_->lRequired & kRequireWaveform) != 0 || visInfo_->lRequired == 0) {
      for (std::size_t i = 0; i < waveform.size(); ++i) {
        const float sample = std::clamp(sampleWaveform(*analysis, i), -1.0f, 1.0f);
        const int value = static_cast<int>(std::lround((sample + 1.0f) * 127.5f));
        waveform[i] = static_cast<std::uint8_t>(std::clamp(value, 0, 255));
      }
    }
    if ((visInfo_->lRequired & kRequireSpectrum) != 0 || visInfo_->lRequired == 0) {
      for (std::size_t i = 0; i < spectrum.size(); ++i) {
        const float magnitude = std::max(sampleSpectrum(*analysis, i), 0.0f);
        const float normalized = magnitude / (magnitude + 1.0f);
        const int value = static_cast<int>(std::lround(normalized * 255.0f));
        spectrum[i] = static_cast<std::uint8_t>(std::clamp(value, 0, 255));
      }
    }
  }

  for (auto& channel : data.Waveform) {
    channel = waveform;
  }
  for (auto& channel : data.Spectrum) {
    channel = spectrum;
  }

  return data;
}

std::filesystem::path SvpLoader::Impl::resolveLibraryPath(const std::string& name) const {
  namespace fs = std::filesystem;
  fs::path input = fs::u8path(name);
  std::vector<fs::path> candidates;

  auto appendVariants = [&candidates](const fs::path& base) {
    if (base.empty()) {
      return;
    }
    candidates.push_back(base);
    if (!base.has_extension()) {
      const std::string baseStr = base.u8string();
      candidates.push_back(fs::u8path(baseStr + ".svp"));
      candidates.push_back(fs::u8path(baseStr + ".uvs"));
      candidates.push_back(fs::u8path(baseStr + ".dll"));
    }
  };

  appendVariants(input);

  std::error_code ec;
  const fs::path cwd = fs::current_path(ec);
  if (!ec) {
    appendVariants(cwd / input);
    appendVariants(cwd / "resources" / input);
    appendVariants(cwd / "resources" / "svp" / input);
  }

  appendVariants(fs::path("resources") / input);
  appendVariants(fs::path("resources") / "svp" / input);

  for (const auto& candidate : candidates) {
    std::error_code existsError;
    if (fs::exists(candidate, existsError) && !existsError &&
        fs::is_regular_file(candidate, existsError) && !existsError) {
      std::error_code canonicalError;
      fs::path canonical = fs::weakly_canonical(candidate, canonicalError);
      if (canonicalError) {
        return candidate;
      }
      return canonical;
    }
  }
  return {};
}

std::string SvpLoader::Impl::settingsFileFor(const std::filesystem::path& library) const {
  namespace fs = std::filesystem;
  const fs::path directory = library.has_parent_path() ? library.parent_path() : fs::current_path();
  const fs::path target = directory / "avs.ini";
  std::error_code canonicalError;
  fs::path canonical = fs::weakly_canonical(target, canonicalError);
  if (canonicalError) {
    return target.u8string();
  }
  return canonical.u8string();
}

#endif  // defined(_WIN32)

SvpLoader::SvpLoader() : impl_(std::make_unique<Impl>()) {}

SvpLoader::~SvpLoader() = default;

void SvpLoader::setParams(const avs::core::ParamBlock& params) { impl_->setParams(params); }

bool SvpLoader::render(avs::core::RenderContext& context) { return impl_->render(context); }

}  // namespace avs::effects::render
