#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace avs::runtime {

struct Heightmap {
  int width = 0;
  int height = 0;
  std::vector<float> samples;

  [[nodiscard]] bool valid() const {
    const std::size_t expected = static_cast<std::size_t>(std::max(width, 0)) *
                                 static_cast<std::size_t>(std::max(height, 0));
    return width > 0 && height > 0 && samples.size() == expected;
  }
};

struct LegacyRenderState {
  std::uint32_t lineBlendMode = 0;
  bool lineBlendModeActive = false;

  void reset() {
    lineBlendMode = 0;
    lineBlendModeActive = false;
  }
};

struct GlobalState {
  static constexpr std::size_t kRegisterCount = 64;

  std::array<double, kRegisterCount> registers{};
  std::unordered_map<std::string, Heightmap> heightmaps;
  LegacyRenderState legacyRender{};

  void reset() {
    registers.fill(0.0);
    heightmaps.clear();
    legacyRender.reset();
  }
};

}  // namespace avs::runtime

