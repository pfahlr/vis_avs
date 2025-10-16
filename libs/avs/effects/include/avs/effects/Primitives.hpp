#pragma once

#include <array>
#include <string>
#include <vector>

#include "avs/core/IEffect.hpp"

namespace avs::effects {

/**
 * @brief Fill a rectangular region with a solid colour.
 */
class PrimitiveSolid : public avs::core::IEffect {
 public:
  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  int x1_{0};
  int y1_{0};
  int x2_{0};
  int y2_{0};
  int color_{0x000000};
  int alpha_{255};
};

/**
 * @brief Render a collection of circular dots.
 */
class PrimitiveDots : public avs::core::IEffect {
 public:
  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  std::vector<std::pair<int, int>> points_{};
  int radius_{1};
  int color_{0xFFFFFF};
  int alpha_{255};
};

/**
 * @brief Render connected line segments.
 */
class PrimitiveLines : public avs::core::IEffect {
 public:
  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  std::vector<std::pair<int, int>> points_{};
  bool closed_{false};
  int width_{1};
  int color_{0xFFFFFF};
  int alpha_{255};
};

/**
 * @brief Rasterise filled or wireframe triangles.
 */
class PrimitiveTriangles : public avs::core::IEffect {
 public:
  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  std::vector<std::array<int, 6>> triangles_{};  // x0,y0,x1,y1,x2,y2
  bool filled_{true};
  int color_{0xFFFFFF};
  int alpha_{255};
  int outlineColor_{0x000000};
  int outlineAlpha_{255};
  int outlineWidth_{0};
};

/**
 * @brief Rounded rectangle primitive.
 */
class PrimitiveRoundedRect : public avs::core::IEffect {
 public:
  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  int x_{0};
  int y_{0};
  int width_{0};
  int height_{0};
  int radius_{0};
  bool filled_{true};
  int color_{0xFFFFFF};
  int alpha_{255};
  int outlineColor_{0x000000};
  int outlineAlpha_{255};
  int outlineWidth_{0};
};

/**
 * @brief Bitmap text renderer with outline and shadow support.
 */
class Text : public avs::core::IEffect {
 public:
  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  std::string text_{};
  int posX_{0};
  int posY_{0};
  int size_{16};
  int widthOverride_{0};
  int spacing_{1};
  int color_{0xFFFFFF};
  int alpha_{255};
  int outlineColor_{0x000000};
  int outlineAlpha_{255};
  int outlineSize_{0};
  bool shadow_{false};
  int shadowColor_{0x000000};
  int shadowAlpha_{128};
  int shadowOffsetX_{2};
  int shadowOffsetY_{2};
  int shadowBlur_{2};
  bool antialias_{false};
  std::string halign_{"left"};
  std::string valign_{"top"};
};

}  // namespace avs::effects

