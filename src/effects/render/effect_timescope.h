#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "avs/core/IEffect.hpp"

namespace avs::effects::render {

class Timescope : public avs::core::IEffect {
 public:
  Timescope();
  ~Timescope() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  enum class BlendMode {
    Replace,
    Additive,
    Average,
    Line,
  };

  void updateColor(std::uint32_t color);
  void resetColumnState(int width);
  void advanceColumn(int width);
  void ensureBandStorage(std::size_t bands);
  void updateSpectrumColumn(const avs::core::RenderContext& context, std::size_t bands);
  [[nodiscard]] std::uint8_t bandIntensityForRow(int row, int height) const;
  void drawPixel(avs::core::RenderContext& context, int x, int y, std::uint8_t intensity) const;
  [[nodiscard]] std::array<std::uint8_t, 3> scaledColor(std::uint8_t intensity) const;

  int currentColumn_ = -1;
  int lastWidth_ = 0;
  bool enabled_ = true;
  BlendMode blendMode_ = BlendMode::Line;
  [[maybe_unused]] int channelMode_ = 2;
  int bandCount_ = 576;

  std::array<std::uint8_t, 3> baseColor_{};
  std::vector<float> smoothedBands_;
  std::vector<std::uint8_t> columnBuffer_;
  float magnitudePeak_ = 1.0f;
};

}  // namespace avs::effects::render
