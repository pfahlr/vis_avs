#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "avs/core/IEffect.hpp"

namespace avs::effects {

class AudioOverlay : public avs::core::IEffect {
 public:
  enum class Mode {
    Wave,
    Spectrum,
    Bands,
    LevelText,
    BandText,
  };

  explicit AudioOverlay(Mode mode);

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  using Color = std::array<std::uint8_t, 4>;

  void drawWave(avs::core::RenderContext& context, const Color& color) const;
  void drawSpectrum(avs::core::RenderContext& context) const;
  void drawBands(avs::core::RenderContext& context) const;
  void drawLevelText(avs::core::RenderContext& context) const;
  void drawBandText(avs::core::RenderContext& context) const;

  static Color parseColor(const avs::core::ParamBlock& params, const std::string& key,
                          Color fallback);
  static Color gradient(float t);
  static void fillRect(avs::core::RenderContext& context, int x0, int y0, int x1, int y1,
                       const Color& color);
  static void putPixel(avs::core::RenderContext& context, int x, int y, const Color& color);
  static void drawLine(avs::core::RenderContext& context, int x0, int y0, int x1, int y1,
                       const Color& color);

  void drawPolyline(avs::core::RenderContext& context, const std::vector<float>& samples,
                    float scaleY, const Color& color) const;
  void drawText(avs::core::RenderContext& context, int originX, int originY,
                const std::string& text, const Color& color) const;

  static std::string formatFloat(float value, int precision = 2);

  Mode mode_;
  Color waveColor_{{255, 255, 255, 255}};
  Color textColor_{{255, 255, 255, 255}};
  float gain_ = 1.0f;
  bool highlightBeat_ = true;
  bool damping_ = true;
  mutable std::vector<float> waveCache_;
  mutable std::vector<float> spectrumCache_;
  mutable std::array<float, 3> bandCache_{{0.0f, 0.0f, 0.0f}};
};

}  // namespace avs::effects
