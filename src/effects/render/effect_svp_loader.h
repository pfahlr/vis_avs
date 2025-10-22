#pragma once

#include <array>
#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>

#include "avs/core/IEffect.hpp"
#include "avs/core/ParamBlock.hpp"
#include "avs/core/RenderContext.hpp"

namespace avs::effects::render {

/**
 * @brief Loader for legacy Sonique Visualization Plug-ins (SVP).
 *
 * The original AVS implementation dynamically loaded SVP DLLs on Windows and
 * forwarded AVS audio buffers to the plug-in's Render callback.  This modern
 * port replicates the behaviour when running on Windows and degrades into a
 * no-op fallback on other platforms so presets continue to parse successfully.
 */
class SvpLoader : public avs::core::IEffect {
 public:
  SvpLoader();
  ~SvpLoader() override;

  void setParams(const avs::core::ParamBlock& params) override;
  bool render(avs::core::RenderContext& context) override;

 private:
  struct SvpVisData {
    unsigned long millSec = 0;
    std::uint8_t waveform[2][512]{};
    std::uint8_t spectrum[2][256]{};
  };

  struct SvpVisInfo {
    unsigned long reserved = 0;
    char* pluginName = nullptr;
    long required = 0;
#ifdef _WIN32
    void(__cdecl* initialize)() = nullptr;
    int(__cdecl* render)(unsigned long*, int, int, int, SvpVisData*) = nullptr;
    int(__cdecl* saveSettings)(char*) = nullptr;
    int(__cdecl* openSettings)(char*) = nullptr;
#else
    void (*initialize)() = nullptr;
    int (*render)(unsigned long*, int, int, int, SvpVisData*) = nullptr;
    int (*saveSettings)(char*) = nullptr;
    int (*openSettings)(char*) = nullptr;
#endif
  };

#ifdef _WIN32
  using QueryModuleFn = SvpVisInfo*(__cdecl*)();
#else
  using QueryModuleFn = SvpVisInfo* (*)();
#endif

  static constexpr long kRequireWaveform = 0x0001;
  static constexpr long kRequireSpectrum = 0x0002;

  static std::string sanitizePath(std::string_view value);
  static std::uint8_t waveformSampleToByte(float sample);
  static std::uint8_t spectrumSampleToByte(float sample);

  void requestReload(const std::string& path);
  void ensureLibraryLoaded();
  void unloadLibrary();
  void updateVisData(const avs::core::RenderContext& context, long requirements);
  void renderFallback(avs::core::RenderContext& context);

  std::mutex mutex_;
  std::string desiredLibrary_;
  bool needsReload_ = true;
  bool settingsOpened_ = false;
  bool initialized_ = false;
  double timeAccumulatorMs_ = 0.0;

  SvpVisData visData_{};

  void* libraryHandle_ = nullptr;
  SvpVisInfo* moduleInfo_ = nullptr;
  std::array<char, 260> settingsFile_{};
};

}  // namespace avs::effects::render
