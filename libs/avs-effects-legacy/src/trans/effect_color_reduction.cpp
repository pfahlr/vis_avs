#include <avs/effects/trans/effect_color_reduction.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace avs::effects::trans {

namespace {
constexpr int kMinLevels = 1;
constexpr int kMaxLevels = 8;
}

ColorReduction::ColorReduction() { updateMask(); }

void ColorReduction::setParams(const avs::core::ParamBlock& params) {
  int requestedLevels = levels_;
  if (params.contains("bits")) {
    requestedLevels = params.getInt("bits", requestedLevels);
  } else if (params.contains("levels")) {
    requestedLevels = params.getInt("levels", requestedLevels);
  } else if (params.contains("bit_depth")) {
    requestedLevels = params.getInt("bit_depth", requestedLevels);
  }

  requestedLevels = std::clamp(requestedLevels, kMinLevels, kMaxLevels);
  if (requestedLevels != levels_) {
    levels_ = requestedLevels;
    updateMask();
  }
}

bool ColorReduction::render(avs::core::RenderContext& context) {
  if (context.width <= 0 || context.height <= 0 || context.framebuffer.data == nullptr ||
      context.framebuffer.size < 4u) {
    return true;
  }

  if (channelMask_ == 0xFFu) {
    return true;
  }

  const std::size_t totalPixels = static_cast<std::size_t>(context.width) *
                                  static_cast<std::size_t>(context.height);
  const std::size_t availablePixels = context.framebuffer.size / 4u;
  const std::size_t pixelCount = std::min(totalPixels, availablePixels);

  std::uint8_t* data = context.framebuffer.data;
  for (std::size_t i = 0; i < pixelCount; ++i) {
    std::uint8_t* px = data + i * 4u;
    px[0] &= channelMask_;
    px[1] &= channelMask_;
    px[2] &= channelMask_;
  }

  return true;
}

void ColorReduction::updateMask() {
  const int clampedLevels = std::clamp(levels_, kMinLevels, kMaxLevels);
  levels_ = clampedLevels;
  const int shift = 8 - clampedLevels;
  channelMask_ = static_cast<std::uint8_t>((0xFFu << shift) & 0xFFu);
}

}  // namespace avs::effects::trans

