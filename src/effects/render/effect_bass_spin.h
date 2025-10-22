#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

#include "avs/core/IEffect.hpp"
#include "avs/core/ParamBlock.hpp"
#include "avs/core/RenderContext.hpp"

namespace avs::audio {
struct Analysis;
}

namespace avs::effects::render {

/**
 * @brief Legacy "Bass Spin" renderer that rotates mirrored spokes using bass energy.
 */
class BassSpin : public avs::core::IEffect {
 public:
  BassSpin();
  ~BassSpin() override = default;

  void setParams(const avs::core::ParamBlock& params) override;
  bool render(avs::core::RenderContext& context) override;

 private:
  struct Color {
    std::uint8_t r = 255;
    std::uint8_t g = 255;
    std::uint8_t b = 255;
    std::uint8_t a = 255;
  };

  struct TrailPoint {
    int x = 0;
    int y = 0;
    bool valid = false;
  };

  enum class Mode { Lines, Triangles };

  static bool hasFramebuffer(const avs::core::RenderContext& context);
  static Color colorFromInt(std::uint32_t value);
  static bool parseColorToken(std::string_view token, Color& color);
  static void writePixel(avs::core::RenderContext& context, int x, int y, const Color& color);
  static void drawLine(avs::core::RenderContext& context, int x0, int y0, int x1, int y1,
                       const Color& color);
  static void drawTriangle(avs::core::RenderContext& context, int x0, int y0, int x1, int y1,
                           int x2, int y2, const Color& color);

  void parseEnabledMask(const avs::core::ParamBlock& params);
  void parseMode(const avs::core::ParamBlock& params);
  void parseColors(const avs::core::ParamBlock& params);
  void clearTrails();

  float computeBassWindow(const avs::audio::Analysis& analysis) const;
  float computeAmplitude(float bassSum);

  static constexpr int kChannelCount = 2;
  static constexpr std::size_t kBassWindow = 44u;

  int enabledMask_ = 0b11;
  std::array<Color, kChannelCount> colors_{};
  Mode mode_ = Mode::Triangles;

  float lastBassSum_ = 0.0f;
  std::array<double, kChannelCount> angles_{};
  std::array<double, kChannelCount> velocities_{};
  std::array<double, kChannelCount> directions_{};
  std::array<std::array<TrailPoint, 2>, kChannelCount> trails_{};
};

}  // namespace avs::effects::render
