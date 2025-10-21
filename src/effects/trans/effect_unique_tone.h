#pragma once

#include <array>
#include <cstdint>
#include <string>

#include "avs/core/IEffect.hpp"

namespace avs::effects::trans {

class UniqueTone : public avs::core::IEffect {
 public:
  UniqueTone();
  ~UniqueTone() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  enum class BlendMode { Replace, Additive, Average };

  void rebuildToneScale();
  static float computeLuminance(float red, float green, float blue);
  static std::uint8_t toByte(float value);
  static std::uint8_t saturatingAdd(std::uint8_t base, std::uint8_t overlay);
  static std::uint8_t blendAverage(std::uint8_t a, std::uint8_t b);
  static BlendMode parseBlendModeValue(const std::string& value, BlendMode fallback);

  BlendMode blendMode_ = BlendMode::Replace;
  int color_ = 0xFFFFFF;
  bool invert_ = false;
  bool enabled_ = true;
  float toneLuminance_ = 0.0f;
  std::array<float, 3> toneScale_{};
};

}  // namespace avs::effects::trans
