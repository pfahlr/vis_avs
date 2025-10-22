#pragma once

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

#include <avs/core/IEffect.hpp>

namespace avs::effects::filters {

class Convolution3x3 : public avs::core::IEffect {
 public:
  Convolution3x3();
  ~Convolution3x3() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  void parseKernel(std::string_view kernelText);

  std::array<float, 9> kernel_{};
  float divisor_ = 1.0f;
  float bias_ = 0.0f;
  bool clampOutput_ = true;
  bool preserveAlpha_ = true;
  std::vector<std::uint8_t> scratch_;
};

}  // namespace avs::effects::filters

