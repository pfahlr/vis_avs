#pragma once

#include <array>
#include <cstdint>

#include <avs/core/IEffect.hpp>

namespace avs::effects::filters {

class Interferences : public avs::core::IEffect {
 public:
  enum class Mode { Add, Subtract, Multiply };

  Interferences() = default;
  ~Interferences() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  int amplitude_ = 64;
  int period_ = 8;
  int speed_ = 1;
  int noise_ = 16;
  int phase_ = 0;
  bool vertical_ = false;
  Mode mode_ = Mode::Add;
  std::array<int, 3> tint_{{255, 255, 255}};
};

}  // namespace avs::effects::filters

