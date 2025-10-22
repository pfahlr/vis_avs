#pragma once

#include <cstdint>
#include <filesystem>
#include <mutex>
#include <string>

#include "avs/core/IEffect.hpp"
#include "avs/core/ParamBlock.hpp"

namespace avs::core {
struct RenderContext;
}

namespace avs::effects::render {

/**
 * @brief Loader for legacy Sonique Visualization Plugin (SVP) modules.
 *
 * The original Winamp AVS implementation dynamically loaded SVP DLLs and
 * delegated rendering to them.  This modern port keeps the API surface while
 * safely degrading to a no-op when dynamic loading is unsupported on the
 * current platform or when the requested module cannot be opened.
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
    std::uint8_t waveform[2][512]{};
    std::uint8_t spectrum[2][256]{};
  };

  struct LegacyVisInfo {
    std::uint32_t reserved = 0;
    const char* pluginName = nullptr;
    std::int32_t requiredFlags = 0;
    void (*initialize)(void) = nullptr;
    int (*render)(std::uint32_t* video, int width, int height, int pitch,
                  LegacyVisData* data) = nullptr;
    int (*saveSettings)(char* fileName) = nullptr;
    int (*openSettings)(char* fileName) = nullptr;
  };

  using QueryModuleFn = LegacyVisInfo* (*)();

  void updateConfigurationLocked(const avs::core::ParamBlock& params);
  void reloadModuleLocked();
  void unloadModuleLocked();
  bool loadModuleLocked(const std::filesystem::path& libraryPath);
  void applySettingsLocked(int (*settingsFn)(char* fileName));
  void ensureInitializedLocked();

  void prepareVisData(const avs::core::RenderContext& context, LegacyVisData& data) const;
  void fillWaveform(const avs::core::RenderContext& context, std::uint8_t (&buffer)[512]) const;
  void fillSpectrum(const avs::core::RenderContext& context, std::uint8_t (&buffer)[256]) const;
  [[nodiscard]] std::filesystem::path resolveLibraryPath(
      const std::filesystem::path& candidate) const;

  mutable std::mutex mutex_;
  std::filesystem::path requestedLibrary_;
  std::filesystem::path baseDirectory_;
  std::filesystem::path loadedLibrary_;
  std::filesystem::path settingsFile_;
  LegacyVisInfo* moduleInfo_ = nullptr;
  void* moduleHandle_ = nullptr;
  bool needsReload_ = false;
  bool initialized_ = false;
  bool enabled_ = true;
  mutable double accumulatedMillis_ = 0.0;
};

}  // namespace avs::effects::render
