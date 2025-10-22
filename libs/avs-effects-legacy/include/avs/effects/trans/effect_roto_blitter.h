#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <avs/core/IEffect.hpp>

namespace avs::effects::trans {

class RotoBlitter : public avs::core::IEffect {
 public:
  RotoBlitter() = default;
  ~RotoBlitter() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  static std::array<float, 2> anchorFromToken(const std::string& token);
  static int clampInt(int value, int minValue, int maxValue);
  static float wrapCoord(float value, float size);
  static int wrapIndex(int value, int size);

  bool ensureHistory(const avs::core::RenderContext& context);
  void storeHistory(const avs::core::RenderContext& context);

  std::array<std::uint8_t, 4> sampleNearest(float x, float y) const;
  std::array<std::uint8_t, 4> sampleBilinear(float x, float y) const;

  int zoomBaseRaw_{31};
  int zoomBeatRaw_{31};
  int zoomCurrentRaw_{31};
  int rotationRaw_{32};
  bool blend_{false};
  bool subpixel_{true};
  bool beatReverse_{false};
  bool beatZoom_{false};
  int reverseSpeed_{0};
  int reverseDirection_{1};
  float reversePos_{1.0f};
  std::array<float, 2> anchorNorm_{0.5f, 0.5f};

  std::vector<std::uint8_t> history_;
  std::vector<std::uint8_t> scratch_;
  int historyWidth_{0};
  int historyHeight_{0};
};

}  // namespace avs::effects::trans

