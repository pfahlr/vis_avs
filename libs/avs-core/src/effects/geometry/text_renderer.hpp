#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include "avs/core.hpp"

namespace avs::effects::geometry::text {

struct RasterOptions {
  int pixelHeight{16};
  int pixelWidth{0};
  int spacing{1};
  bool antialias{false};
};

struct Surface {
  int width{0};
  int height{0};
  std::vector<std::uint8_t> mask;
};

class Renderer {
 public:
  Surface render(std::string_view text, const RasterOptions& options) const;
};

}  // namespace avs::effects::geometry::text
