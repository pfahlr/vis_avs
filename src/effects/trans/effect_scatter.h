#pragma once

#include <cstdint>
#include <vector>

#include "avs/core/IEffect.hpp"

namespace avs::effects::trans {

/**
 * @brief Implements the legacy "Trans / Scatter" effect.
 *
 * Pixels are displaced by sampling a random neighbourhood with a soft falloff
 * near the framebuffer edges. The random source is tied to the deterministic
 * frame RNG so results are reproducible for a fixed seed.
 */
class Scatter : public avs::core::IEffect {
 public:
  Scatter() = default;
  ~Scatter() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  static bool hasFramebuffer(const avs::core::RenderContext& context);
  static std::uint32_t lerpColor(std::uint32_t original,
                                 std::uint32_t scattered,
                                 int weight,
                                 int scale);

  void ensureScratch(std::size_t bytes);
  void ensureOffsetTable(int width);

  struct ScatterOffset {
    int dx = 0;
    int dy = 0;
  };

  bool enabled_ = true;
  int cachedWidth_ = 0;
  std::vector<ScatterOffset> offsets_;
  std::vector<std::uint8_t> scratch_;
};

}  // namespace avs::effects::trans

