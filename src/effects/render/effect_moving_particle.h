#pragma once

#include <array>
#include <cstdint>

#include "avs/core/IEffect.hpp"
#include "avs/core/ParamBlock.hpp"
#include "avs/core/RenderContext.hpp"

namespace avs::effects::render {

/**
 * @brief Legacy "Moving Particle" renderer with velocity damping and beat-driven scaling.
 */
class MovingParticle : public avs::core::IEffect {
 public:
  MovingParticle();
  ~MovingParticle() override = default;

  void setParams(const avs::core::ParamBlock& params) override;
  bool render(avs::core::RenderContext& context) override;

 private:
  struct Color {
    std::uint8_t r = 255;
    std::uint8_t g = 255;
    std::uint8_t b = 255;
    std::uint8_t a = 255;
  };

  static bool hasFramebuffer(const avs::core::RenderContext& context);
  static Color colorFromInt(std::uint32_t value);
  static Color colorFromString(const std::string& value, Color fallback);
  static void applyBlend(std::uint8_t* pixel, const Color& color, int mode);
  static std::uint8_t saturatingAdd(std::uint8_t base, std::uint8_t add);

  void parseParams(const avs::core::ParamBlock& params);
  int randomInt(avs::core::RenderContext& context, int min, int max);
  void drawCircle(avs::core::RenderContext& context, int centerX, int centerY, int diameter) const;

  static constexpr int kMaxSize = 256;

  int enabledMask_ = 1;
  Color color_{};
  int maxDistance_ = 16;
  int baseSize_ = 8;
  int beatSize_ = 8;
  int blendMode_ = 1;
  int sizePosition_ = 8;

  std::array<double, 2> target_{};
  std::array<double, 2> velocity_{};
  std::array<double, 2> position_{};
};

}  // namespace avs::effects::render
