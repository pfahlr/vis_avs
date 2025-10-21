#pragma once

#include <array>
#include <cstdint>
#include <string>

#include "avs/core/IEffect.hpp"
#include "avs/core/ParamBlock.hpp"

namespace avs::effects::trans {

class ColorModifier : public avs::core::IEffect {
 public:
  enum class Mode {
    Sine = 0,
    Cosine = 1,
    SineCosine = 2,
  };

  ColorModifier();
  ~ColorModifier() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  void updateLookupTables();
  static float clamp01(float value);
  static Mode modeFromInt(int value);
  static Mode modeFromString(const std::string& value, Mode fallback);

  Mode mode_ = Mode::Sine;
  bool dirty_ = true;
  std::array<std::uint8_t, 256> redLut_{};
  std::array<std::uint8_t, 256> greenLut_{};
  std::array<std::uint8_t, 256> blueLut_{};
};

}  // namespace avs::effects::trans
