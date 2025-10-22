#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#if defined(_WIN32)
#  include <filesystem>
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#endif

#include <avs/core/IEffect.hpp>
#include <avs/core/ParamBlock.hpp>
#include <avs/core/RenderContext.hpp>

namespace avs::effects::render {

/**
 * @brief Loads legacy SVP visualisation plug-ins and renders them into the
 *        current frame. When SVP modules are unavailable, the effect renders
 *        an empty frame as a graceful fallback.
 */
class SvpLoader : public avs::core::IEffect {
 public:
  SvpLoader();
  ~SvpLoader() override;

  void setParams(const avs::core::ParamBlock& params) override;
  bool render(avs::core::RenderContext& context) override;

 private:
  void renderFallback(avs::core::RenderContext& context);

#if defined(_WIN32)
  struct VisData {
    unsigned long MillSec = 0;
    unsigned char Waveform[2][512]{};
    unsigned char Spectrum[2][256]{};
  };

  struct VisInfo {
    unsigned long Reserved = 0;
    char* PluginName = nullptr;
    long lRequired = 0;
    void(__cdecl* Initialize)(void) = nullptr;
    BOOL(__cdecl* Render)(unsigned long*, int, int, int, VisData*) = nullptr;
    BOOL(__cdecl* SaveSettings)(char*) = nullptr;
    BOOL(__cdecl* OpenSettings)(char*) = nullptr;
  };

  bool ensurePluginLoaded();
  bool loadPlugin(const std::filesystem::path& candidatePath);
  void unloadPlugin();
  void updateAudioData(const avs::core::RenderContext& context);
  static void convertBufferToAbgr(std::uint32_t* pixels, std::size_t count);
  static void convertBufferToRgba(std::uint32_t* pixels, std::size_t count);

  std::filesystem::path loadedPath_{};
  HMODULE module_ = nullptr;
  VisInfo* visInfo_ = nullptr;
  VisData visData_{};
#endif

  std::string library_;
  bool reloadPending_ = false;
  bool fallbackNotified_ = false;
};

}  // namespace avs::effects::render
