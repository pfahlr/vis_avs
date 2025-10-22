#pragma once

#include <cstdint>
#include <vector>

#include <avs/core/IEffect.hpp>

namespace avs::effects::filters {

class Grain : public avs::core::IEffect {
 public:
  Grain() = default;
  ~Grain() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  void regenerateStaticPattern(int width, int height, std::uint64_t seedBase);

  int amount_ = 16;
  bool monochrome_ = false;
  bool staticGrain_ = false;
  int seedOffset_ = 0;
  bool dirty_ = true;

  int patternWidth_ = 0;
  int patternHeight_ = 0;
  std::uint64_t patternSeed_ = 0;
  std::vector<int> staticPattern_;
};

}  // namespace avs::effects::filters

