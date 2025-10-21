#pragma once

#include <cstdint>
#include <string>

#include "avs/core/IEffect.hpp"

namespace avs::effects::misc {

class RenderMode : public avs::core::IEffect {
 public:
  RenderMode();
  ~RenderMode() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  void applyRaw(std::uint32_t rawValue);
  [[nodiscard]] std::uint32_t encodeRaw() const;
  void setModeFromInt(int modeValue);
  void setModeFromString(const std::string& token);

  bool enabled_ = true;
  std::uint8_t mode_ = 0;
  std::uint8_t adjustableAlpha_ = 0;
  std::uint8_t lineWidth_ = 1;
};

}  // namespace avs::effects::misc

