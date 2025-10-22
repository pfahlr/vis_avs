#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>

#include "avs/core/IEffect.hpp"

namespace avs::core {
class ParamBlock;
}

namespace avs::runtime {
struct LegacyRenderState;
}

namespace avs::effects::render {

/**
 * @brief Scrolling waveform waterfall similar to the classic AVS Timescope effect.
 */
class Timescope : public avs::core::IEffect {
 public:
  Timescope();
  ~Timescope() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  enum class Channel { Left, Right, Mix };
  enum class BlendMode { Replace = 0, Additive = 1, Line = 2 };
  enum class Operation { Replace, Additive, Average, Line };

  struct Rgba {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 255;
  };

  static Rgba scaleColor(const Rgba& base, float intensity);
  static Rgba decodeColor(std::uint32_t value);
  static std::uint32_t clampBands(int requested);
  static bool parseColorString(std::string_view value, Rgba& color);

  [[nodiscard]] Operation resolveOperation(const avs::runtime::LegacyRenderState* legacy) const;
  static void applyReplace(std::uint8_t* pixel, const Rgba& color);
  static void applyAdditive(std::uint8_t* pixel, const Rgba& color);
  static void applyAverage(std::uint8_t* pixel, const Rgba& color);
  static void applyLine(std::uint8_t* pixel, const Rgba& color,
                        const avs::runtime::LegacyRenderState* legacy);
  static void applyLineBlend(std::uint8_t* pixel, const Rgba& color, int mode, int parameter);
  static std::uint8_t saturatingAdd(std::uint8_t base, std::uint8_t add);
  static std::uint8_t saturatingSub(std::uint8_t base, std::uint8_t sub);
  static std::uint8_t multiplyChannel(std::uint8_t a, std::uint8_t b);

  float sampleBand(const avs::core::RenderContext& context, int band, std::size_t totalBands) const;
  float sampleWaveform(const float* waveform, std::size_t size, std::size_t band,
                       std::size_t totalBands) const;
  float sampleSpectrum(const float* spectrum, std::size_t size, std::size_t band,
                       std::size_t totalBands) const;
  [[nodiscard]] std::pair<std::size_t, std::size_t> channelRange(std::size_t total) const;

  bool enabled_ = true;
  Channel channel_ = Channel::Mix;
  BlendMode blendMode_ = BlendMode::Line;
  bool blendAverage_ = false;
  int bandCount_ = 576;
  int columnCursor_ = -1;
  Rgba color_{};
};

}  // namespace avs::effects::render
