#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

namespace avs::effects::text {

struct RasterOptions {
  int pixelHeight{16};
  int pixelWidth{0};
  int spacing{1};
  bool antialias{false};
};

struct Surface {
  int width{0};
  int height{0};
  std::vector<std::uint8_t> mask;  // Row-major alpha coverage 0..255.
};

class TextRenderer {
 public:
  Surface render(std::string_view text, const RasterOptions& options) const;
};

}  // namespace avs::effects::text

