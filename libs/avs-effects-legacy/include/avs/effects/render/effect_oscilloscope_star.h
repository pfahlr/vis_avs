#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "avs/effects/legacy_effect.h"

namespace avs::effects::render {

class EffectOscilloscopeStar : public LegacyEffect {
 public:
  static constexpr std::size_t kMaxColors = 16;

  EffectOscilloscopeStar();

  void render(LegacyRenderContext& context) override;
  void loadConfig(const std::uint8_t* data, std::size_t size) override;
  std::vector<std::uint8_t> saveConfig() const override;

  std::uint32_t effectFlags() const { return effect_; }
  std::uint32_t colorCount() const { return numColors_; }
  const std::array<std::uint32_t, kMaxColors>& colors() const { return colors_; }
  std::uint32_t size() const { return size_; }
  std::uint32_t rotation() const { return rotation_; }

 private:
  std::uint32_t effect_ = 0;
  std::uint32_t numColors_ = 0;
  std::array<std::uint32_t, kMaxColors> colors_{};
  std::uint32_t size_ = 0;
  std::uint32_t rotation_ = 0;
};

}  // namespace avs::effects::render
