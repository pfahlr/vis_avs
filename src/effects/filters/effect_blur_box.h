#pragma once

#include <cstdint>
#include <vector>

#include "avs/core/IEffect.hpp"

namespace avs::effects::filters {

class BlurBox : public avs::core::IEffect {
 public:
  BlurBox() = default;
  ~BlurBox() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  void ensureBuffers(int width, int height);
  void horizontalPass(const std::uint8_t* src, std::uint8_t* dst, int width, int height) const;
  void verticalPass(const std::uint8_t* src, std::uint8_t* dst, int width, int height) const;

  int radius_ = 1;
  bool preserveAlpha_ = true;
  mutable std::vector<std::uint8_t> scratch_;
  mutable std::vector<int> prefixRow_;
  mutable std::vector<int> prefixColumn_;
};

}  // namespace avs::effects::filters

