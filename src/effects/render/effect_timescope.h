#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "avs/core/IEffect.hpp"
#include "avs/core/ParamBlock.hpp"
#include "avs/core/RenderContext.hpp"

namespace avs::effects::render {

/**
 * @brief Scrolling visualization of the audio waveform.
 */
class Timescope : public avs::core::IEffect {
 public:
  Timescope();
  ~Timescope() override = default;

  void setParams(const avs::core::ParamBlock& params) override;
  bool render(avs::core::RenderContext& context) override;

 private:
  enum class BlendMode {
    Replace,
    Additive,
    Average,
    Alpha,
  };

  struct Color {
    std::uint8_t r = 255;
    std::uint8_t g = 255;
    std::uint8_t b = 255;
  };

  struct Rgba {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 255;
  };

  struct SampleView {
    const float* data = nullptr;
    std::size_t size = 0;
  };

  static std::uint8_t clampByte(int value);
  static std::uint8_t saturatingAdd(std::uint8_t a, std::uint8_t b);
  static Color decodeColor(std::uint32_t value);
  static std::uint32_t encodeColor(const Color& color);
  static std::uint8_t intensityFromSample(float sample);
  static std::string toLower(std::string_view value);

  void resetState();
  void applyBlend(std::uint8_t* pixel, const Rgba& src) const;
  Rgba colorForIntensity(std::uint8_t intensity) const;
  SampleView resolveWaveform(const avs::core::RenderContext& context) const;
  std::size_t sampleIndexForRow(int row, int height, std::size_t available) const;

  bool enabled_ = true;
  Color color_{};
  BlendMode blendMode_ = BlendMode::Alpha;
  int bands_ = 576;
  int column_ = -1;
  int lastWidth_ = 0;
};

}  // namespace avs::effects::render
