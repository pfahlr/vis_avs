#include "avs/effects.hpp"

#include <algorithm>

namespace avs {

namespace {
std::uint8_t clampChannel(int value) {
  return static_cast<std::uint8_t>(std::clamp(value, 0, 255));
}
}  // namespace

FadeoutEffect::FadeoutEffect(int fadeLen, std::uint32_t color) {
  fadeLen_ = std::clamp(fadeLen, 0, 255);
  targetR_ = static_cast<std::uint8_t>(color & 0xFFu);
  targetG_ = static_cast<std::uint8_t>((color >> 8) & 0xFFu);
  targetB_ = static_cast<std::uint8_t>((color >> 16) & 0xFFu);
  recomputeLuts();
}

void FadeoutEffect::init(int w, int h) {
  (void)w;
  (void)h;
}

void FadeoutEffect::recomputeLuts() {
  for (int value = 0; value < 256; ++value) {
    int r = value;
    if (r <= targetR_ - fadeLen_) {
      r += fadeLen_;
    } else if (r >= targetR_ + fadeLen_) {
      r -= fadeLen_;
    } else {
      r = targetR_;
    }
    lutR_[static_cast<size_t>(value)] = clampChannel(r);

    int g = value;
    if (g <= targetG_ - fadeLen_) {
      g += fadeLen_;
    } else if (g >= targetG_ + fadeLen_) {
      g -= fadeLen_;
    } else {
      g = targetG_;
    }
    lutG_[static_cast<size_t>(value)] = clampChannel(g);

    int b = value;
    if (b <= targetB_ - fadeLen_) {
      b += fadeLen_;
    } else if (b >= targetB_ + fadeLen_) {
      b -= fadeLen_;
    } else {
      b = targetB_;
    }
    lutB_[static_cast<size_t>(value)] = clampChannel(b);
  }
}

void FadeoutEffect::process(const Framebuffer& in, Framebuffer& out) {
  out.w = in.w;
  out.h = in.h;
  out.rgba.resize(in.rgba.size());
  if (fadeLen_ == 0) {
    out.rgba = in.rgba;
    return;
  }

  const std::uint8_t* src = in.rgba.data();
  std::uint8_t* dst = out.rgba.data();
  size_t pixels = in.rgba.size() / 4;
  for (size_t i = 0; i < pixels; ++i) {
    size_t idx = i * 4;
    dst[idx + 0] = lutR_[src[idx + 0]];
    dst[idx + 1] = lutG_[src[idx + 1]];
    dst[idx + 2] = lutB_[src[idx + 2]];
    dst[idx + 3] = src[idx + 3];
  }
}

}  // namespace avs
