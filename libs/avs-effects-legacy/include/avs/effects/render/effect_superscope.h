#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <avs/core/IEffect.hpp>
#include <avs/core/ParamBlock.hpp>
#include <avs/core/RenderContext.hpp>

namespace avs::effects::render {

/**
 * @brief SuperScope - Programmable waveform renderer using EEL scripts.
 *
 * SuperScope is one of AVS's most powerful effects, allowing users to write
 * custom EEL scripts to generate arbitrary waveforms and visualizations.
 *
 * Architecture:
 * - 4 EEL script contexts: init, frame, beat, point
 * - Audio-reactive via getspec/getosc functions (TODO: implement)
 * - Per-point rendering with customizable position and color
 * - Two rendering modes: dots (individual points) and lines (connected)
 *
 * EEL Variables:
 * - n: number of points to render (set by init/frame scripts)
 * - b: beat flag (1.0 on beat, 0.0 otherwise)
 * - x, y: output coordinates in normalized space [-1, 1]
 * - i: current point index [0, 1]
 * - v: audio waveform value for current point [-1, 1]
 * - w, h: framebuffer width and height in pixels
 * - red, green, blue: color values [0, 1]
 * - skip: if non-zero, skip rendering this point
 * - linesize: line thickness (for line mode)
 * - drawmode: 0 = dots, 1 = lines
 *
 * TODO: Full implementation requires:
 * - EEL VM integration (libs/third_party/ns-eel)
 * - Audio data access (waveform/spectrum from RenderContext)
 * - Variable binding between C++ and EEL
 * - Script compilation and execution
 * - Color palette cycling
 */
class SuperScopeEffect : public avs::core::IEffect {
 public:
  SuperScopeEffect();
  ~SuperScopeEffect() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

  // Script management
  void setInitScript(const std::string& script) { initScript_ = script; }
  void setFrameScript(const std::string& script) { frameScript_ = script; }
  void setBeatScript(const std::string& script) { beatScript_ = script; }
  void setPointScript(const std::string& script) { pointScript_ = script; }

  // Configuration
  void setDrawMode(int mode) { drawMode_ = mode; }  // 0=dots, 1=lines
  void setColors(const std::vector<std::uint32_t>& colors) { colors_ = colors; }
  void setAudioChannel(int channel) { audioChannel_ = channel; }

 private:
  // EEL scripts (4 contexts)
  std::string initScript_;   // Executed once at start
  std::string frameScript_;  // Executed each frame
  std::string beatScript_;   // Executed on beat
  std::string pointScript_;  // Executed per point

  // Configuration
  int drawMode_ = 0;                              // 0=dots, 1=lines
  std::vector<std::uint32_t> colors_{0xFFFFFF};  // Color palette
  int audioChannel_ = 2;                          // 0=left, 1=right, 2=center
  int colorPos_ = 0;                              // Current position in color cycle

  // EEL VM state (TODO: implement)
  // NSEEL_VMCTX eel_vm_ = nullptr;
  // NSEEL_CODEHANDLE eel_init_ = nullptr;
  // NSEEL_CODEHANDLE eel_frame_ = nullptr;
  // NSEEL_CODEHANDLE eel_beat_ = nullptr;
  // NSEEL_CODEHANDLE eel_point_ = nullptr;

  // EEL variables (TODO: bind to VM)
  double n_ = 100.0;         // Number of points
  double b_ = 0.0;           // Beat flag
  double x_ = 0.0;           // Output x coordinate
  double y_ = 0.0;           // Output y coordinate
  double i_ = 0.0;           // Point index [0,1]
  double v_ = 0.0;           // Audio value [-1,1]
  double w_ = 0.0;           // Width
  double h_ = 0.0;           // Height
  double red_ = 1.0;         // Red [0,1]
  double green_ = 1.0;       // Green [0,1]
  double blue_ = 1.0;        // Blue [0,1]
  double skip_ = 0.0;        // Skip flag
  double linesize_ = 1.0;    // Line thickness
  double drawmode_ = 0.0;    // Draw mode

  bool inited_ = false;
  int lastX_ = 0;
  int lastY_ = 0;

  // Helper methods
  std::uint32_t getCurrentColor();
  void advanceColorCycle();
  void renderDot(avs::core::RenderContext& context, int x, int y, std::uint32_t color);
  void renderLine(avs::core::RenderContext& context, int x1, int y1, int x2, int y2,
                  std::uint32_t color, int thickness);
};

}  // namespace avs::effects::render
