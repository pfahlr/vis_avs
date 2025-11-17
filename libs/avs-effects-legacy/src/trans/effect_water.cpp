#include <avs/effects/trans/effect_water.h>

#include <cstring>

namespace avs::effects::trans {

namespace {
constexpr std::size_t kChannels = 4u;

std::uint8_t clampByte(int value) {
  if (value <= 0) {
    return 0;
  }
  if (value >= 255) {
    return 255;
  }
  return static_cast<std::uint8_t>(value);
}

bool hasFramebuffer(const avs::core::RenderContext& context, std::size_t requiredBytes) {
  return context.framebuffer.data != nullptr && context.framebuffer.size >= requiredBytes &&
         context.width > 0 && context.height > 0;
}

}  // namespace

void Water::setParams(const avs::core::ParamBlock& params) {
  if (params.contains("enabled")) {
    enabled_ = params.getBool("enabled", enabled_);
  }
}

bool Water::render(avs::core::RenderContext& context) {
  if (!enabled_) {
    return true;
  }

  if (context.width <= 0 || context.height <= 0) {
    return true;
  }

  const std::size_t width = static_cast<std::size_t>(context.width);
  const std::size_t height = static_cast<std::size_t>(context.height);
  const std::size_t requiredBytes = width * height * kChannels;
  if (!hasFramebuffer(context, requiredBytes)) {
    return true;
  }

  if (lastFrame_.size() != requiredBytes) {
    lastFrame_.assign(requiredBytes, 0u);
  }
  if (scratch_.size() < requiredBytes) {
    scratch_.resize(requiredBytes);
  }

  const std::uint8_t* src = context.framebuffer.data;
  const std::uint8_t* prev = lastFrame_.data();
  std::uint8_t* dst = scratch_.data();

  const int widthInt = context.width;
  const int heightInt = context.height;

  for (int y = 0; y < heightInt; ++y) {
    for (int x = 0; x < widthInt; ++x) {
      const std::size_t offset =
          (static_cast<std::size_t>(y) * width + static_cast<std::size_t>(x)) * kChannels;
      int sumR = 0;
      int sumG = 0;
      int sumB = 0;
      int contributions = 0;

      auto accumulate = [&](int nx, int ny) {
        if (nx < 0 || nx >= widthInt || ny < 0 || ny >= heightInt) {
          return;
        }
        const std::size_t neighborOffset =
            (static_cast<std::size_t>(ny) * width + static_cast<std::size_t>(nx)) * kChannels;
        sumR += static_cast<int>(src[neighborOffset + 0]);
        sumG += static_cast<int>(src[neighborOffset + 1]);
        sumB += static_cast<int>(src[neighborOffset + 2]);
        ++contributions;
      };

      accumulate(x - 1, y);
      accumulate(x + 1, y);
      accumulate(x, y - 1);
      accumulate(x, y + 1);

      if (contributions >= 3) {
        sumR /= 2;
        sumG /= 2;
        sumB /= 2;
      }

      const int valueR = sumR - static_cast<int>(prev[offset + 0]);
      const int valueG = sumG - static_cast<int>(prev[offset + 1]);
      const int valueB = sumB - static_cast<int>(prev[offset + 2]);

      dst[offset + 0] = clampByte(valueR);
      dst[offset + 1] = clampByte(valueG);
      dst[offset + 2] = clampByte(valueB);
      dst[offset + 3] = src[offset + 3];
    }
  }

  std::memcpy(lastFrame_.data(), dst, requiredBytes);
  std::memcpy(context.framebuffer.data, dst, requiredBytes);

  return true;
}

}  // namespace avs::effects::trans
