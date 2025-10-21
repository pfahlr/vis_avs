#pragma once

#include <array>
#include <cstdint>
#include <string_view>

#include "avs/core/IEffect.hpp"

namespace avs::effects::render {

struct Rgb {
  std::uint8_t r = 0;
  std::uint8_t g = 0;
  std::uint8_t b = 0;
};

class DotPlane : public avs::core::IEffect {
 public:
  DotPlane();
  ~DotPlane() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  static constexpr int kGridSize = 64;

  static std::uint8_t clampByte(int value);
  static std::uint8_t saturatingAdd(std::uint8_t base, std::uint8_t add);

  void rebuildColorGradient();
  bool applyColorParam(const avs::core::ParamBlock& params, std::string_view prefix,
                       std::size_t index);
  static Rgb decodeColor(int value);
  static std::uint32_t encodeColor(const Rgb& color);

  void sampleAudio(const avs::core::RenderContext& context,
                   std::array<float, kGridSize>& amplitudes) const;
  void updateHeightField(const std::array<float, kGridSize>& previousTop,
                         const std::array<float, kGridSize>& newTop);

  std::array<float, kGridSize * kGridSize> height_{};
  std::array<float, kGridSize * kGridSize> velocity_{};
  std::array<std::uint32_t, kGridSize * kGridSize> colorRows_{};
  std::array<std::uint32_t, 64> colorGradient_{};
  std::array<Rgb, 5> palette_{};

  float rotationDegrees_ = 0.0f;
  int rotationVelocity_ = 16;
  int tiltDegrees_ = -20;
  bool paletteDirty_ = true;
};

}  // namespace avs::effects::render
