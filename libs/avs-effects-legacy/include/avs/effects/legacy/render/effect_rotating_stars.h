#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <avs/core/IEffect.hpp>

namespace avs::effects::render {

class RotatingStars : public avs::core::IEffect {
 public:
  RotatingStars();
  ~RotatingStars() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  using Color = std::array<std::uint8_t, 4>;

  static constexpr int kMaxPaletteSize = 16;
  static constexpr int kColorCycleLength = 64;

  static Color makeColor(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255u);
  static Color makeColorFromInt(std::uint32_t packed);
  static void putPixel(avs::core::RenderContext& context, int x, int y, const Color& color);
  static void drawLine(avs::core::RenderContext& context, int x0, int y0, int x1, int y1,
                       const Color& color);

  void updatePalette(const avs::core::ParamBlock& params);
  [[nodiscard]] Color currentColor();
  [[nodiscard]] std::array<float, 2> computeChannelAmplitudes(
      const avs::core::RenderContext& context) const;
  [[nodiscard]] float computeSpectrumPeak(const float* spectrum, std::size_t size) const;

  std::vector<Color> palette_;
  int colorPos_ = 0;
  double rotation_ = 0.0;
  double rotationSpeed_ = 0.1;
};

}  // namespace avs::effects::render
