#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "avs/core/IEffect.hpp"
#include "avs/core/ParamBlock.hpp"
#include "avs/core/RenderContext.hpp"

namespace avs::effects::trans {

/**
 * @brief Randomly displaces pixels within a small neighbourhood.
 *
 * This effect reproduces the behaviour of the legacy R_Scat module by sampling
 * nearby pixels with a soft falloff that favours the current location.  The
 * top and bottom four rows are preserved exactly to match the original
 * implementation.
 */
class Scatter : public avs::core::IEffect {
 public:
  Scatter();
  ~Scatter() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  static constexpr int kNeighbourTableSize = 512;
  static constexpr int kBorderRows = 4;

  bool enabled_ = true;
  std::vector<std::uint8_t> scratch_;
  std::array<int, kNeighbourTableSize> offsetX_{};
  std::array<int, kNeighbourTableSize> offsetY_{};
};

}  // namespace avs::effects::trans
