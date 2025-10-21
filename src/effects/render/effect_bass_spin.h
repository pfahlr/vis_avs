#pragma once

#include <array>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>

#include "avs/core/IEffect.hpp"

namespace avs::effects::render {

class BassSpin : public avs::core::IEffect {
 public:
  BassSpin();
  ~BassSpin() override = default;

  void setParams(const avs::core::ParamBlock& params) override;
  bool render(avs::core::RenderContext& context) override;

 private:
  struct ArmPoint {
    int x = 0;
    int y = 0;
    bool valid = false;
  };

  struct SideState {
    std::array<ArmPoint, 2> lastPoints{};
    double angle = 0.0;
    double velocity = 0.0;
  };

  using Color = std::array<std::uint8_t, 4>;

  void resetCachedState(int width, int height);
  static Color parseColor(const avs::core::ParamBlock& params,
                          const std::initializer_list<std::string>& keys, Color fallback);
  static Color parseColorString(std::string_view value, Color fallback);
  static Color colorFromInt(int rgb, Color fallback);
  static void putPixel(avs::core::RenderContext& context, int x, int y, const Color& color);
  static void drawLine(avs::core::RenderContext& context, int x0, int y0, int x1, int y1,
                       const Color& color);
  static void drawTriangle(avs::core::RenderContext& context, int x0, int y0, int x1, int y1,
                           int x2, int y2, const Color& color);

  double computeBassWindow(const avs::core::RenderContext& context) const;
  void drawArms(avs::core::RenderContext& context, int centerX, int centerY, int offsetX,
                int offsetY, SideState& state, const Color& color);

  std::array<SideState, 2> sides_{};
  std::array<Color, 2> colors_{{Color{255, 255, 255, 255}, Color{255, 255, 255, 255}}};
  int enabledMask_ = 0x3;
  bool triangles_ = true;
  double lastBassSum_ = 0.0;
  int lastWidth_ = 0;
  int lastHeight_ = 0;
};

}  // namespace avs::effects::render
