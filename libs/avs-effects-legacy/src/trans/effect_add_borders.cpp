#include <avs/effects/trans/effect_add_borders.h>

#include <algorithm>
#include <cstring>

namespace avs::effects::trans {

AddBorders::AddBorders() = default;

std::uint32_t AddBorders::makeRGBA(std::uint32_t rgb) {
  // Convert RGB (0xRRGGBB) to RGBA (0xAABBGGRR with full opacity)
  std::uint8_t r = static_cast<std::uint8_t>((rgb >> 16) & 0xFF);
  std::uint8_t g = static_cast<std::uint8_t>((rgb >> 8) & 0xFF);
  std::uint8_t b = static_cast<std::uint8_t>(rgb & 0xFF);

  // Return as RGBA bytes in framebuffer format (RGBA)
  return (0xFF << 24) | (b << 16) | (g << 8) | r;
}

bool AddBorders::render(avs::core::RenderContext& context) {
  if (!enabled_ || !context.framebuffer.data) {
    return true;
  }

  const int w = context.width;
  const int h = context.height;

  if (w <= 0 || h <= 0 || size_ <= 0) {
    return true;
  }

  // Calculate border dimensions (percentage-based)
  int borderHeight = (h * size_) / 100;
  int borderWidth = (w * size_) / 100;

  // Ensure minimum 1 pixel border
  if (borderHeight < 1) borderHeight = 1;
  if (borderWidth < 1) borderWidth = 1;

  // Don't draw borders larger than half the frame
  borderHeight = std::min(borderHeight, h / 2);
  borderWidth = std::min(borderWidth, w / 2);

  const std::uint32_t rgba = makeRGBA(color_);
  std::uint8_t* fb = context.framebuffer.data;

  // Draw top border
  for (int y = 0; y < borderHeight; ++y) {
    for (int x = 0; x < w; ++x) {
      const std::size_t offset = (static_cast<std::size_t>(y) * w + x) * 4;
      std::memcpy(fb + offset, &rgba, 4);
    }
  }

  // Draw bottom border
  for (int y = h - borderHeight; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      const std::size_t offset = (static_cast<std::size_t>(y) * w + x) * 4;
      std::memcpy(fb + offset, &rgba, 4);
    }
  }

  // Draw left border (excluding corners already drawn)
  for (int y = borderHeight; y < h - borderHeight; ++y) {
    for (int x = 0; x < borderWidth; ++x) {
      const std::size_t offset = (static_cast<std::size_t>(y) * w + x) * 4;
      std::memcpy(fb + offset, &rgba, 4);
    }
  }

  // Draw right border (excluding corners already drawn)
  for (int y = borderHeight; y < h - borderHeight; ++y) {
    for (int x = w - borderWidth; x < w; ++x) {
      const std::size_t offset = (static_cast<std::size_t>(y) * w + x) * 4;
      std::memcpy(fb + offset, &rgba, 4);
    }
  }

  return true;
}

void AddBorders::setParams(const avs::core::ParamBlock& params) {
  if (params.contains("enabled")) {
    enabled_ = params.getBool("enabled", enabled_);
  }

  if (params.contains("color")) {
    color_ = static_cast<std::uint32_t>(params.getInt("color", static_cast<int>(color_)));
  }

  if (params.contains("size")) {
    size_ = params.getInt("size", size_);
    size_ = std::clamp(size_, 0, 100);
  }
}

}  // namespace avs::effects::trans
