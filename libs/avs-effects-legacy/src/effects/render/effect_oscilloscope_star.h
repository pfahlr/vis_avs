#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <avs/core/IEffect.hpp>

namespace avs::effects::render {

class OscilloscopeStar : public avs::core::IEffect {
 public:
  OscilloscopeStar();
  ~OscilloscopeStar() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  using Color = std::array<std::uint8_t, 4>;

  enum class ChannelMode { Left, Right, Center };
  enum class AnchorMode { Left, Center, Right };

  static constexpr int kMaxPaletteSize = 16;
  static constexpr int kColorCycleLength = 64;
  static constexpr int kArmCount = 5;
  static constexpr int kSegmentsPerArm = 64;
  static constexpr std::size_t kWaveformSamples = 576u;

  static Color makeColor(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255u);
  static Color makeColorFromInt(std::uint32_t packed);
  static void putPixel(avs::core::RenderContext& context, int x, int y, const Color& color);
  static void drawLine(avs::core::RenderContext& context, int x0, int y0, int x1, int y1, const Color& color);

  void updatePalette(const avs::core::ParamBlock& params);
  void applyEffectBits(int effect);
  void setChannelFromString(const std::string& value);
  void setAnchorFromString(const std::string& value);
  void updateRotationSpeedFromLegacy(float legacyRot);
  [[nodiscard]] Color currentColor();
  [[nodiscard]] std::array<float, kWaveformSamples> sampleWaveform(
      const avs::core::RenderContext& context) const;
  static float interpolateSample(const std::array<float, kWaveformSamples>& samples, double position);

  [[nodiscard]] double anchorX(const avs::core::RenderContext& context) const;

  std::vector<Color> palette_;
  int colorPos_ = 0;
  double rotation_ = 0.0;
  double rotationSpeed_ = 0.03;
  int sizeParam_ = 8;
  ChannelMode channel_ = ChannelMode::Center;
  AnchorMode anchor_ = AnchorMode::Center;
};

}  // namespace avs::effects::render
