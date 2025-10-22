#include <avs/effects/legacy/dynamic/frame_warp.h>

#include <algorithm>
#include <cmath>

namespace avs::effects {

bool FrameWarpEffect::prepareHistory(const avs::core::RenderContext& context) {
  if (context.width <= 0 || context.height <= 0 || !context.framebuffer.data) {
    return false;
  }
  width_ = context.width;
  height_ = context.height;
  const std::size_t expectedSize = static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_) * 4u;
  if (context.framebuffer.size < expectedSize) {
    return false;
  }
  if (history_.size() != expectedSize) {
    history_.assign(context.framebuffer.data, context.framebuffer.data + expectedSize);
  }
  return true;
}

void FrameWarpEffect::storeHistory(const avs::core::RenderContext& context) {
  if (width_ <= 0 || height_ <= 0 || !context.framebuffer.data) {
    return;
  }
  const std::size_t expectedSize = static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_) * 4u;
  if (history_.size() != expectedSize) {
    history_.resize(expectedSize);
  }
  std::copy(context.framebuffer.data, context.framebuffer.data + expectedSize, history_.begin());
}

FrameWarpEffect::Rgba FrameWarpEffect::sampleHistory(float normX, float normY, bool wrap) const {
  if (history_.empty() || width_ <= 0 || height_ <= 0) {
    return {0, 0, 0, 255};
  }
  // Transform normalized coordinates (-1..1) into pixel space.
  float u = (normX + 1.0f) * 0.5f;
  float v = (1.0f - (normY + 1.0f) * 0.5f);
  float fx = u * static_cast<float>(width_ - 1);
  float fy = v * static_cast<float>(height_ - 1);

  if (wrap) {
    fx = wrapCoord(fx, static_cast<float>(width_));
    fy = wrapCoord(fy, static_cast<float>(height_));
  } else {
    fx = std::clamp(fx, 0.0f, static_cast<float>(width_ - 1));
    fy = std::clamp(fy, 0.0f, static_cast<float>(height_ - 1));
  }

  return bilinearSample(fx, fy, wrap);
}

FrameWarpEffect::Rgba FrameWarpEffect::bilinearSample(float fx, float fy, bool wrap) const {
  const int x0 = static_cast<int>(std::floor(fx));
  const int y0 = static_cast<int>(std::floor(fy));
  const float tx = fx - static_cast<float>(x0);
  const float ty = fy - static_cast<float>(y0);

  const int x1 = wrap ? wrapIndex(x0 + 1, width_) : std::min(x0 + 1, width_ - 1);
  const int y1 = wrap ? wrapIndex(y0 + 1, height_) : std::min(y0 + 1, height_ - 1);
  const int ix0 = wrap ? wrapIndex(x0, width_) : std::clamp(x0, 0, width_ - 1);
  const int iy0 = wrap ? wrapIndex(y0, height_) : std::clamp(y0, 0, height_ - 1);

  const auto fetch = [&](int px, int py) {
    const std::size_t index = (static_cast<std::size_t>(py) * static_cast<std::size_t>(width_) +
                               static_cast<std::size_t>(px)) * 4u;
    return Rgba{history_[index + 0], history_[index + 1], history_[index + 2], history_[index + 3]};
  };

  const Rgba c00 = fetch(ix0, iy0);
  const Rgba c10 = fetch(x1, iy0);
  const Rgba c01 = fetch(ix0, y1);
  const Rgba c11 = fetch(x1, y1);

  const auto lerp = [](float a, float b, float t) { return a + (b - a) * t; };
  Rgba result{};
  for (int i = 0; i < 4; ++i) {
    const float c0 = lerp(static_cast<float>(c00[static_cast<std::size_t>(i)]),
                          static_cast<float>(c10[static_cast<std::size_t>(i)]), tx);
    const float c1 = lerp(static_cast<float>(c01[static_cast<std::size_t>(i)]),
                          static_cast<float>(c11[static_cast<std::size_t>(i)]), tx);
    const float value = lerp(c0, c1, ty);
    const float rounded = static_cast<float>(std::round(value));
    const float clamped = std::clamp(rounded, 0.0f, 255.0f);
    result[static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(clamped);
  }
  return result;
}

int FrameWarpEffect::wrapIndex(int value, int size) {
  if (size <= 0) {
    return 0;
  }
  int wrapped = value % size;
  if (wrapped < 0) {
    wrapped += size;
  }
  return wrapped;
}

float FrameWarpEffect::wrapCoord(float value, float size) {
  if (size <= 0.0f) {
    return 0.0f;
  }
  float wrapped = std::fmod(value, size);
  if (wrapped < 0.0f) {
    wrapped += size;
  }
  return wrapped;
}

}  // namespace avs::effects

