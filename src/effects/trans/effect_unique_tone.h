#pragma once

#include <array>
#include <cstdint>

#include "avs/core/IEffect.hpp"

namespace avs::effects::trans {

class UniqueTone : public avs::core::IEffect {
 public:
  UniqueTone();
  ~UniqueTone() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  enum class BlendMode { kReplace, kAdditive, kAverage };

  void rebuildLookupTables();

  static bool hasFramebuffer(const avs::core::RenderContext& context);
  static std::uint8_t saturatingAdd(std::uint8_t base, std::uint8_t addend);

  bool enabled_ = true;
  bool invert_ = false;
  BlendMode blendMode_ = BlendMode::kReplace;
  std::uint32_t color_ = 0x00FFFFFFu;
  std::array<std::uint8_t, 3> toneColor_{{0xFFu, 0xFFu, 0xFFu}};
  std::array<std::uint8_t, 256> redTable_{};
  std::array<std::uint8_t, 256> greenTable_{};
  std::array<std::uint8_t, 256> blueTable_{};
  bool tablesDirty_ = true;
};

}  // namespace avs::effects::trans
