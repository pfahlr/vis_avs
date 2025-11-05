#pragma once

#include <cstdint>
#include <vector>

#include "avs/effects/legacy_effect.h"

namespace avs::effects::trans {

class EffectFadeout : public LegacyEffect {
 public:
  EffectFadeout();

  void render(LegacyRenderContext& context) override;
  void loadConfig(const std::uint8_t* data, std::size_t size) override;
  std::vector<std::uint8_t> saveConfig() const override;

  std::uint32_t radius() const { return fadelen_; }
  std::uint32_t color() const { return color_; }

 private:
  std::uint32_t fadelen_ = 16u;
  std::uint32_t color_ = 0u;
};

}  // namespace avs::effects::trans
