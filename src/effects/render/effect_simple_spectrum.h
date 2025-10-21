#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "avs/core/IEffect.hpp"
#include "avs/core/ParamBlock.hpp"

namespace avs::effects::render {

class SimpleSpectrum : public avs::core::IEffect {
 public:
  SimpleSpectrum();
  ~SimpleSpectrum() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  struct Color {
    std::uint8_t r = 255;
    std::uint8_t g = 255;
    std::uint8_t b = 255;
    std::uint8_t a = 255;
  };

  void updateEffectBits(const avs::core::ParamBlock& params);
  void updateColors(const avs::core::ParamBlock& params);
  [[nodiscard]] Color currentColor() const;
  void advanceColorCycle();
  void prepareAudioData(const avs::core::RenderContext& context);

  static void putPixel(std::uint8_t* framebuffer, std::size_t bufferSize, int width, int height,
                       int x, int y, const Color& color);
  static void drawLine(std::uint8_t* framebuffer, std::size_t bufferSize, int width, int height,
                       int x0, int y0, int x1, int y1, const Color& color);
  static void drawVerticalLine(std::uint8_t* framebuffer, std::size_t bufferSize, int width,
                               int height, int x, int y0, int y1, const Color& color);

  int effectBits_ = (2 << 2) | (2 << 4);
  std::vector<Color> colors_;
  int colorPosition_ = 0;

  std::array<float, 576> analyzerLeft_{};
  std::array<float, 576> analyzerRight_{};
  std::array<float, 576> analyzerCenter_{};
  std::array<float, 576> scopeLeft_{};
  std::array<float, 576> scopeRight_{};
  std::array<float, 576> scopeCenter_{};
};

}  // namespace avs::effects::render

using Effect_RenderSimple = avs::effects::render::SimpleSpectrum;

