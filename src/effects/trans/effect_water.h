#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "avs/core/IEffect.hpp"

namespace avs::effects::trans {

class Water : public avs::core::IEffect {
 public:
  Water() = default;
  ~Water() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  bool enabled_ = true;
  std::vector<std::uint8_t> lastFrame_;
  std::vector<std::uint8_t> scratch_;
};

}  // namespace avs::effects::trans
