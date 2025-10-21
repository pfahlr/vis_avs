#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "avs/core/IEffect.hpp"

namespace avs::effects::render {

class OscilloscopeStar : public avs::core::IEffect {
 public:
  OscilloscopeStar() = default;
  ~OscilloscopeStar() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  using Color = std::array<std::uint8_t, 4>;

  enum class Channel {
    Left = 0,
    Right = 1,
    Center = 2,
  };

  enum class Anchor {
    Left = 0,
    Right = 1,
    Center = 2,
  };

  static constexpr int kArms = 5;
  static constexpr int kSegments = 64;

  static Color makeColor(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255);
  static Color colorFromRgbInt(std::uint32_t rgb);
  static void putPixel(avs::core::RenderContext& context, int x, int y, const Color& color);
  static void drawLine(avs::core::RenderContext& context, int x0, int y0, int x1, int y1,
                       const Color& color);

  static std::optional<Color> tryParseColor(const avs::core::ParamBlock& params,
                                            const std::string& key);
  void parseColors(const avs::core::ParamBlock& params);
  void advanceColor();
  [[nodiscard]] Color currentColor() const;

  Channel channel_ = Channel::Center;
  Anchor anchor_ = Anchor::Center;
  int size_ = 8;                // legacy range 0..32
  float rotationSpeed_ = 3.0f;  // matches legacy default (3/100 turn per frame)
  double rotationAngle_ = 0.0;
  int colorPhase_ = 0;
  std::vector<Color> palette_ = {makeColor(255, 255, 255)};
};

}  // namespace avs::effects::render
