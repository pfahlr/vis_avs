#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "avs/core/IEffect.hpp"
#include "avs/core/ParamBlock.hpp"
#include "avs/core/RenderContext.hpp"

namespace avs::effects::render {

/**
 * @brief Scrolling spectrogram visualizing the audio spectrum over time.
 */
class Timescope : public avs::core::IEffect {
 public:
  Timescope();
  ~Timescope() override = default;

  void setParams(const avs::core::ParamBlock& params) override;
  bool render(avs::core::RenderContext& context) override;

 private:
  struct Color {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
  };

  enum class BlendMode { Replace, Additive, Line };

  static bool hasFramebuffer(const avs::core::RenderContext& context);
  static std::uint8_t saturatingAdd(std::uint8_t base, std::uint8_t add);
  static Color colorFromInt(std::uint32_t value);
  static bool parseColorToken(std::string_view token, std::uint32_t& value);
  static std::uint32_t parseColorString(const std::string& value, std::uint32_t fallback);

  void applyParams(const avs::core::ParamBlock& params);
  void ensureBandCapacity();
  void updateSpectrumState(const avs::core::RenderContext& context);
  void decaySpectrumState();
  [[nodiscard]] Color scaleColor(float intensity) const;
  void applyBlend(std::uint8_t* pixel, const Color& color) const;

  bool enabled_ = true;
  Color color_{};
  BlendMode blendMode_ = BlendMode::Line;
  bool blendAverage_ = false;
  int channelSelection_ = 2;
  int bandCount_ = 576;
  int cursor_ = -1;
  float normalization_ = 1.0f;
  std::vector<float> bandState_;
  std::vector<float> scratchBands_;
};

}  // namespace avs::effects::render
