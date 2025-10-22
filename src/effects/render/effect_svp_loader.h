#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

#include "avs/core/IEffect.hpp"
#include "avs/core/ParamBlock.hpp"
#include "avs/core/RenderContext.hpp"

namespace avs::effects::render {

class SvpLoader : public avs::core::IEffect {
 public:
  SvpLoader();
  ~SvpLoader() override;

  void setParams(const avs::core::ParamBlock& params) override;
  bool render(avs::core::RenderContext& context) override;

 private:
  struct VisData {
    std::uint32_t millisecond = 0;
    std::array<std::array<std::uint8_t, 512>, 2> waveform{};
    std::array<std::array<std::uint8_t, 256>, 2> spectrum{};
  };

  struct VisInfo {
    std::uint32_t reserved = 0;
    char* pluginName = nullptr;
    std::int32_t required = 0;
    void (*initialize)() = nullptr;
    int (*render)(std::uint32_t* video, int width, int height, int pitch, VisData* data) = nullptr;
    int (*saveSettings)(char* fileName) = nullptr;
    int (*openSettings)(char* fileName) = nullptr;
  };

  using QueryModuleFn = VisInfo* (*)();
  using LibraryHandle = void*;

  void requestLibraryReload(const std::string& path);
  void unloadLibrary();
  void ensureLibraryLoaded();
  bool loadLibrary(const std::filesystem::path& path);
  [[nodiscard]] bool hasValidFramebuffer(const avs::core::RenderContext& context) const;
  void ensurePixelBuffer(std::size_t pixelCount);
  void prepareVisData(const avs::core::RenderContext& context);
  void updateWaveform(const avs::core::RenderContext& context);
  void updateSpectrum(const avs::core::RenderContext& context);
  static std::uint32_t rgbaToAbgr(const std::uint8_t* pixel);
  static void abgrToRgba(std::uint32_t value, std::uint8_t* pixel);
  static std::filesystem::path resolveLibraryPath(const std::string& requested);
  [[nodiscard]] bool needsWaveform() const;
  [[nodiscard]] bool needsSpectrum() const;

  std::mutex mutex_;
  std::string requestedLibrary_;
  std::filesystem::path resolvedLibrary_;
  std::string settingsPath_;
  bool libraryDirty_ = false;
  LibraryHandle libraryHandle_ = nullptr;
  VisInfo* visInfo_ = nullptr;
  std::vector<std::uint32_t> pixelBuffer_;
  VisData visData_{};
  double elapsedMilliseconds_ = 0.0;
};

}  // namespace avs::effects::render
