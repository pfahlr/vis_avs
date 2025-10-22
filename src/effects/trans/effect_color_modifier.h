#pragma once

#include <array>
#include <cstdint>

#include "avs/core/IEffect.hpp"

namespace avs::effects::trans {

/**
 * @brief Applies sinusoidal color remapping similar to the legacy Color Modifier effect.
 */
class ColorModifier : public avs::core::IEffect {
 public:
  enum class Mode {
    Identity = 0,
    Sine = 1,
    Cosine = 2,
    SineCosine = 3,
  };

  ColorModifier() = default;
  ~ColorModifier() override = default;

  void setParams(const avs::core::ParamBlock& params) override;
  bool render(avs::core::RenderContext& context) override;

 private:
  void recomputeLookupTables();

  bool enabled_ = true;
  Mode mode_ = Mode::Sine;
  bool tablesDirty_ = true;
  std::array<std::uint8_t, 256> redTable_{};
  std::array<std::uint8_t, 256> greenTable_{};
  std::array<std::uint8_t, 256> blueTable_{};
};

}  // namespace avs::effects::trans
