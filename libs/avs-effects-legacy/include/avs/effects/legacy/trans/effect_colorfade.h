#pragma once

#include <array>

#include <avs/core/IEffect.hpp>

namespace avs::effects::trans {

class Colorfade : public avs::core::IEffect {
 public:
  Colorfade();
  ~Colorfade() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  static constexpr int kMinOffset = -32;
  static constexpr int kMaxOffset = 32;

  static int clampOffset(int value);

  void updateOffsets(avs::core::RenderContext& context);

  bool enabled_ = true;
  bool randomizeOnBeat_ = false;
  bool smooth_ = false;

  std::array<int, 3> baseOffsets_{};
  std::array<int, 3> beatOffsets_{};
  std::array<int, 3> currentOffsets_{};
};

}  // namespace avs::effects::trans
