#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

#include "avs/core/RenderContext.hpp"

namespace avs::effects::filters {

inline bool hasFramebuffer(const avs::core::RenderContext& context) {
  return context.framebuffer.data != nullptr && context.framebuffer.size >= 4u &&
         context.width > 0 && context.height > 0;
}

inline std::uint8_t clampByte(int value) {
  return static_cast<std::uint8_t>(value <= 0 ? 0 : (value >= 255 ? 255 : value));
}

inline std::uint8_t saturatingAdd(std::uint8_t base, int delta) {
  const int value = static_cast<int>(base) + delta;
  return clampByte(value);
}

inline void ensureScratch(std::vector<std::uint8_t>& scratch, std::size_t required) {
  if (scratch.size() < required) {
    scratch.resize(required);
  }
}

inline int clampIndex(int value, int minValue, int maxValue) {
  return value < minValue ? minValue : (value > maxValue ? maxValue : value);
}

}  // namespace avs::effects::filters

