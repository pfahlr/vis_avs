#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

#include "avs/core/IEffect.hpp"

namespace avs::core {
struct RenderContext;
class ParamBlock;
}  // namespace avs::core

class Effect_RenderSvpLoader : public avs::core::IEffect {
 public:
  Effect_RenderSvpLoader();
  ~Effect_RenderSvpLoader() override;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  static constexpr std::size_t kWaveformSamples = 512;
  static constexpr std::size_t kSpectrumSamples = 256;

  struct SvpVisData {
    std::uint32_t millis = 0;
    std::uint8_t waveform[2][kWaveformSamples]{};
    std::uint8_t spectrum[2][kSpectrumSamples]{};
  };

  void clearVisData();
  void applyParameters(const avs::core::ParamBlock& params);
  bool ensureFramebuffer(const avs::core::RenderContext& context);
  bool ensurePluginLoaded();
  bool tryLoadPlugin();
  bool tryLoadPluginFromPath(const std::filesystem::path& path);
  std::optional<std::filesystem::path> resolveLibraryPath() const;
  void unloadPlugin();
  void updateVisData(const avs::core::RenderContext& context);

  std::string requestedLibrary_;
  std::filesystem::path loadedLibraryPath_;
  std::string lastErrorMessage_;

  bool libraryDirty_ = false;
  bool attemptedLoad_ = false;
  bool loadSuccessful_ = false;
  bool missingFramebufferLogged_ = false;
  bool missingLibraryLogged_ = false;
  bool unsupportedLogged_ = false;

  SvpVisData visData_{};

  void* moduleHandle_ = nullptr;
  void* pluginInfoHandle_ = nullptr;
};
