#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include "avs/core/IEffect.hpp"

namespace avs::effects::render {

/**
 * @brief Loads legacy Sonique Visualization Plug-in (SVP/UVS) modules.
 *
 * The loader mirrors the behaviour of the original Winamp AVS "Render / SVP Loader"
 * component.  When a plug-in library is available it is dynamically loaded and used to
 * render into the current framebuffer.  On platforms without dynamic library support, or
 * when the requested plug-in cannot be found, the effect gracefully degrades to a no-op so
 * presets can continue executing.
 */
class SvpLoader : public avs::core::IEffect {
 public:
  SvpLoader();
  ~SvpLoader() override;

  void setParams(const avs::core::ParamBlock& params) override;
  bool render(avs::core::RenderContext& context) override;

 private:
  struct LegacyVisData {
    std::uint32_t millisecond = 0;
    std::array<std::array<std::uint8_t, 512>, 2> waveform{};
    std::array<std::array<std::uint8_t, 256>, 2> spectrum{};
  };

  struct LegacyVisInfo {
    using LegacyBool = int;
    using InitializeFn = void (*)();
    using RenderFn = LegacyBool (*)(std::uint32_t*, int, int, int, LegacyVisData*);
    using SettingsFn = LegacyBool (*)(char*);

    std::uint32_t reserved = 0;
    const char* pluginName = nullptr;
    std::int32_t requiredFlags = 0;
    InitializeFn initialize = nullptr;
    RenderFn render = nullptr;
    SettingsFn saveSettings = nullptr;
    SettingsFn openSettings = nullptr;
  };

  using QueryModuleFn = LegacyVisInfo* (*)();
  using LibraryHandle = void*;

  void reloadLibraryLocked();
  void unloadLibraryLocked();
  std::string resolveLibraryPath(const std::string& candidate) const;
  void updateConfigBuffer(const std::string& libraryPath);
  void populateVisData(const avs::core::RenderContext& context, LegacyVisData& data);
  static std::uint8_t convertWaveformSample(float value);
  static std::uint8_t convertSpectrumSample(float value);
  static bool hasFramebuffer(const avs::core::RenderContext& context);

  std::mutex mutex_;
  std::string requestedLibrary_;
  std::string loadedLibraryPath_;
  std::vector<char> configFileBuffer_;
  LibraryHandle libraryHandle_ = nullptr;
  LegacyVisInfo* visInfo_ = nullptr;
  QueryModuleFn queryModule_ = nullptr;
  bool libraryDirty_ = false;
  bool startTimeInitialized_ = false;
  std::chrono::steady_clock::time_point startTime_{};
};

}  // namespace avs::effects::render

