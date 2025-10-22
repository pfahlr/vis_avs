#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "avs/core/IEffect.hpp"
#include "avs/core/ParamBlock.hpp"
#include "avs/core/RenderContext.hpp"

namespace avs::effects::render {

/**
 * @brief Legacy-inspired bar and scope spectrum visualizer.
 */
class SimpleSpectrum : public avs::core::IEffect {
 public:
  SimpleSpectrum();
  ~SimpleSpectrum() override = default;

  void setParams(const avs::core::ParamBlock& params) override;
  bool render(avs::core::RenderContext& context) override;

 private:
  struct Color {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
  };

  struct Rgba {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 255;
  };

  enum class Mode { SolidAnalyzer = 0, LineAnalyzer = 1, LineScope = 2, SolidScope = 3 };
  enum class DotMode { Analyzer, Scope };

  static constexpr int kMaxColors = 16;
  static constexpr int kColorCycle = 64;
  static constexpr int kAnalyzerBands = 200;
  static constexpr int kWaveformSamples = 288;
  static constexpr float kSpectrumDecay = 0.88f;
  static constexpr float kNormalizationDecay = 0.96f;

  static std::string toLower(std::string_view value);
  static bool parseColorToken(std::string_view token, Color& color);
  static Color colorFromInt(std::uint32_t value);

  static void blendPixel(avs::core::RenderContext& context, int x, int y, const Rgba& color);
  static void drawLine(avs::core::RenderContext& context, int x0, int y0, int x1, int y1,
                       const Rgba& color);
  static void drawVerticalLine(avs::core::RenderContext& context, int x, int y0, int y1,
                               const Rgba& color);

  void parseEffectBits(const avs::core::ParamBlock& params);
  void parseColors(const avs::core::ParamBlock& params);
  void normalizeColorCursor();

  Color currentColor();

  [[nodiscard]] Mode mode() const;
  [[nodiscard]] DotMode dotMode() const;
  [[nodiscard]] int placement() const;

  void updateSpectrumState(const avs::core::RenderContext& context);
  void decaySpectrumState();
  [[nodiscard]] float sampleSpectrum(float index) const;
  void sampleWaveform(const avs::core::RenderContext& context,
                      std::array<float, kWaveformSamples>& samples) const;
  [[nodiscard]] float sampleWaveformAt(const std::array<float, kWaveformSamples>& samples,
                                       float index) const;

  void renderDotAnalyzer(avs::core::RenderContext& context, const Rgba& color, float yscale);
  void renderDotScope(avs::core::RenderContext& context, const Rgba& color, float yscale);
  void renderSolidAnalyzer(avs::core::RenderContext& context, const Rgba& color, float yscale);
  void renderLineAnalyzer(avs::core::RenderContext& context, const Rgba& color, float yscale);
  void renderLineScope(avs::core::RenderContext& context, const Rgba& color, float yscale);
  void renderSolidScope(avs::core::RenderContext& context, const Rgba& color, float yscale);

  int effectBits_ = 0;
  std::vector<Color> colors_;
  int colorCursor_ = 0;
  std::array<float, kAnalyzerBands> spectrumState_{};
  float normalization_ = 1.0f;
};

}  // namespace avs::effects::render

