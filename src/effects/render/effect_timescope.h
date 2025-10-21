#pragma once

#include <cstdint>

#include "audio/analyzer.h"
#include "avs/core/IEffect.hpp"
#include "avs/core/ParamBlock.hpp"
#include "avs/core/RenderContext.hpp"

namespace avs::effects::render {

class Timescope : public avs::core::IEffect {
 public:
  Timescope();
  ~Timescope() override = default;

  void setParams(const avs::core::ParamBlock& params) override;
  bool render(avs::core::RenderContext& context) override;

 private:
  enum class BlendMode {
    Replace,
    Additive,
    Average,
    Max,
  };

  [[nodiscard]] int nextCursor(int width);
  void applyBlend(std::uint8_t* pixel, std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) const;
  void updateBlendModeFromParams(const avs::core::ParamBlock& params);

  bool enabled_ = true;
  int color_ = 0x00FFFFFF;
  BlendMode blendMode_ = BlendMode::Replace;
  int channel_ = 2;
  int sampleCount_ = static_cast<int>(avs::audio::Analysis::kWaveformSize);
  float thickness_ = 0.05f;
  int cursor_ = 0;
  int lastWidth_ = -1;
};

}  // namespace avs::effects::render

