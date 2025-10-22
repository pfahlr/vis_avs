#pragma once

#include <cstddef>
#include <cstdint>

#include "avs/core/IEffect.hpp"
#include "avs/core/ParamBlock.hpp"
#include "avs/core/RenderContext.hpp"

namespace avs::effects::render {

/**
 * @brief Scrolling column-based waveform visualization similar to the legacy AVS Timescope.
 */
class Timescope : public avs::core::IEffect {
 public:
  Timescope();
  ~Timescope() override = default;

  void setParams(const avs::core::ParamBlock& params) override;
  bool render(avs::core::RenderContext& context) override;

 private:
  struct Rgba {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 255;
  };

  enum class BlendMode { Replace, Additive, Average, Line };
  enum class Channel { Left, Right, Mix };

  static Rgba decodeColor(std::uint32_t color);
  static Rgba scaleColor(const Rgba& color, float intensity);
  static std::uint8_t clampByte(int value);

  void writePixel(std::uint8_t* pixel, const Rgba& color) const;
  void addPixel(std::uint8_t* pixel, const Rgba& color) const;
  void averagePixel(std::uint8_t* pixel, const Rgba& color) const;
  void lineBlendPixel(std::uint8_t* pixel, const Rgba& color) const;

  float sampleValue(const avs::core::RenderContext& context, std::size_t index) const;

  bool enabled_ = true;
  BlendMode blendMode_ = BlendMode::Line;
  [[maybe_unused]] Channel channel_ = Channel::Mix;
  std::uint32_t color_ = 0x00FFFFFFu;
  int bands_ = 576;
  int column_ = -1;
  int lineBlendStrength_ = 192;
};

}  // namespace avs::effects::render
