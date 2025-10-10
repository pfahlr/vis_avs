#include "avs/core.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace avs {

namespace {

constexpr int kChannelCount = 4;

inline uint8_t clampToByte(int value) {
  return static_cast<uint8_t>(std::clamp(value, 0, 255));
}

inline uint8_t clampToByte(float value) {
  return clampToByte(static_cast<int>(std::lround(std::clamp(value, 0.0f, 255.0f))));
}

void wrapCoordinate(float& coord, int size, Wrap mode) {
  if (size <= 0) {
    coord = 0.0f;
    return;
  }

  switch (mode) {
    case Wrap::Clamp: {
      coord = std::clamp(coord, 0.0f, static_cast<float>(size - 1));
      break;
    }
    case Wrap::Wrap: {
      const float sizeF = static_cast<float>(size);
      coord = std::fmod(coord, sizeF);
      if (coord < 0.0f) coord += sizeF;
      break;
    }
    case Wrap::Mirror: {
      if (size == 1) {
        coord = 0.0f;
        return;
      }
      const float period = static_cast<float>(size - 1) * 2.0f;
      float mirrored = std::fmod(std::fabs(coord), period);
      if (mirrored < 0.0f) mirrored += period;
      if (mirrored > static_cast<float>(size - 1)) {
        mirrored = period - mirrored;
      }
      coord = mirrored;
      break;
    }
  }
}

ColorRGBA8 readPixel(const FrameBufferView& src, int x, int y) {
  if (!src.data || src.width <= 0 || src.height <= 0) {
    return {};
  }
  x = std::clamp(x, 0, src.width - 1);
  y = std::clamp(y, 0, src.height - 1);
  const auto* row = src.data + static_cast<size_t>(y) * static_cast<size_t>(src.stride);
  const auto* px = row + static_cast<size_t>(x) * kChannelCount;
  return {px[0], px[1], px[2], px[3]};
}

}  // namespace

ColorRGBA8 sampleRGBA(const FrameBufferView& src,
                      float x,
                      float y,
                      const SampleOptions& opt) {
  if (!src.data || src.width <= 0 || src.height <= 0) {
    return {};
  }

  float fx = x;
  float fy = y;
  wrapCoordinate(fx, src.width, opt.wrap);
  wrapCoordinate(fy, src.height, opt.wrap);

  if (opt.filter == Filter::Nearest) {
    const int ix = static_cast<int>(std::lround(fx));
    const int iy = static_cast<int>(std::lround(fy));
    return readPixel(src, ix, iy);
  }

  const int x0 = static_cast<int>(std::floor(fx));
  const int y0 = static_cast<int>(std::floor(fy));

  float fx1 = fx + 1.0f;
  float fy1 = fy + 1.0f;
  wrapCoordinate(fx1, src.width, opt.wrap);
  wrapCoordinate(fy1, src.height, opt.wrap);
  if (opt.wrap == Wrap::Wrap && src.height > 1 && fy1 < fy) {
    fy1 = static_cast<float>(y0);
  }
  const int x1 = static_cast<int>(std::floor(fx1));
  const int y1 = static_cast<int>(std::floor(fy1));

  const float tx = fx - static_cast<float>(x0);
  const float ty = fy - static_cast<float>(y0);

  const ColorRGBA8 c00 = readPixel(src, x0, y0);
  const ColorRGBA8 c10 = readPixel(src, x1, y0);
  const ColorRGBA8 c01 = readPixel(src, x0, y1);
  const ColorRGBA8 c11 = readPixel(src, x1, y1);

  const auto lerp = [](float a, float b, float t) { return a + (b - a) * t; };

  const ColorRGBA8 cx0{
      clampToByte(lerp(static_cast<float>(c00.r), static_cast<float>(c10.r), tx)),
      clampToByte(lerp(static_cast<float>(c00.g), static_cast<float>(c10.g), tx)),
      clampToByte(lerp(static_cast<float>(c00.b), static_cast<float>(c10.b), tx)),
      clampToByte(lerp(static_cast<float>(c00.a), static_cast<float>(c10.a), tx))};
  const ColorRGBA8 cx1{
      clampToByte(lerp(static_cast<float>(c01.r), static_cast<float>(c11.r), tx)),
      clampToByte(lerp(static_cast<float>(c01.g), static_cast<float>(c11.g), tx)),
      clampToByte(lerp(static_cast<float>(c01.b), static_cast<float>(c11.b), tx)),
      clampToByte(lerp(static_cast<float>(c01.a), static_cast<float>(c11.a), tx))};

  return {
      clampToByte(lerp(static_cast<float>(cx0.r), static_cast<float>(cx1.r), ty)),
      clampToByte(lerp(static_cast<float>(cx0.g), static_cast<float>(cx1.g), ty)),
      clampToByte(lerp(static_cast<float>(cx0.b), static_cast<float>(cx1.b), ty)),
      clampToByte(lerp(static_cast<float>(cx0.a), static_cast<float>(cx1.a), ty)),
  };
}

void blendPixel(ColorRGBA8& dst, const ColorRGBA8& src, BlendMode mode) {
  switch (mode) {
    case BlendMode::Replace:
      dst = src;
      break;
    case BlendMode::Add:
      dst.r = clampToByte(static_cast<int>(dst.r) + static_cast<int>(src.r));
      dst.g = clampToByte(static_cast<int>(dst.g) + static_cast<int>(src.g));
      dst.b = clampToByte(static_cast<int>(dst.b) + static_cast<int>(src.b));
      break;
    case BlendMode::Subtract:
      dst.r = clampToByte(static_cast<int>(dst.r) - static_cast<int>(src.r));
      dst.g = clampToByte(static_cast<int>(dst.g) - static_cast<int>(src.g));
      dst.b = clampToByte(static_cast<int>(dst.b) - static_cast<int>(src.b));
      break;
    case BlendMode::Multiply:
      dst.r = clampToByte(static_cast<float>(dst.r) * static_cast<float>(src.r) / 255.0f);
      dst.g = clampToByte(static_cast<float>(dst.g) * static_cast<float>(src.g) / 255.0f);
      dst.b = clampToByte(static_cast<float>(dst.b) * static_cast<float>(src.b) / 255.0f);
      break;
    case BlendMode::Max:
      dst.r = std::max(dst.r, src.r);
      dst.g = std::max(dst.g, src.g);
      dst.b = std::max(dst.b, src.b);
      break;
    case BlendMode::Min:
      dst.r = std::min(dst.r, src.r);
      dst.g = std::min(dst.g, src.g);
      dst.b = std::min(dst.b, src.b);
      break;
    case BlendMode::Average:
      dst.r = static_cast<uint8_t>((static_cast<int>(dst.r) + static_cast<int>(src.r)) / 2);
      dst.g = static_cast<uint8_t>((static_cast<int>(dst.g) + static_cast<int>(src.g)) / 2);
      dst.b = static_cast<uint8_t>((static_cast<int>(dst.b) + static_cast<int>(src.b)) / 2);
      break;
    case BlendMode::Xor:
      dst.r ^= src.r;
      dst.g ^= src.g;
      dst.b ^= src.b;
      break;
    case BlendMode::Fifty:
      dst.r = static_cast<uint8_t>((static_cast<int>(dst.r) + static_cast<int>(src.r)) / 2);
      dst.g = static_cast<uint8_t>((static_cast<int>(dst.g) + static_cast<int>(src.g)) / 2);
      dst.b = static_cast<uint8_t>((static_cast<int>(dst.b) + static_cast<int>(src.b)) / 2);
      break;
  }

  const int invAlpha = 255 - src.a;
  dst.a = clampToByte(static_cast<int>(src.a) + static_cast<int>(dst.a) * invAlpha / 255);
}

}  // namespace avs

