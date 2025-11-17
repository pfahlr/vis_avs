#include <avs/effects/trans/effect_blur.h>

#include <algorithm>
#include <cstring>

namespace {
constexpr int kMaxRadius = 32;

bool hasFramebuffer(const avs::core::RenderContext& context) {
  return context.framebuffer.data != nullptr && context.framebuffer.size >= 4u && context.width > 0 &&
         context.height > 0;
}

int convertStrength(float value) {
  if (value <= 0.0f) {
    return 0;
  }
  if (value >= 256.0f) {
    return 256;
  }
  if (value <= 1.0f) {
    return static_cast<int>(std::clamp(value, 0.0f, 1.0f) * 256.0f + 0.5f);
  }
  return static_cast<int>(value + 0.5f);
}

}  // namespace

namespace avs::effects::trans {

bool R_Blur::render(avs::core::RenderContext& context) {
  if (!hasFramebuffer(context)) {
    return true;
  }
  return renderBox(context);
}

void R_Blur::setParams(const avs::core::ParamBlock& params) {
  const bool hasRadius = params.contains("radius");
  const bool hasStrength = params.contains("strength");
  if (params.contains("roundmode")) {
    roundMode_ = params.getBool("roundmode", roundMode_);
  } else if (params.contains("round_mode")) {
    roundMode_ = params.getBool("round_mode", roundMode_);
  }

  if (hasRadius) {
    radius_ = std::clamp(params.getInt("radius", radius_), 0, kMaxRadius);
  }
  if (hasStrength) {
    const float strengthValue = params.getFloat("strength", static_cast<float>(strength_));
    strength_ = std::clamp(convertStrength(strengthValue), 0, 256);
  }
  if (params.contains("enabled")) {
    const int legacyMode = std::clamp(params.getInt("enabled", 1), 0, 3);
    switch (legacyMode) {
      case 0:
        strength_ = 0;
        break;
      case 1:
        radius_ = std::max(1, radius_);
        if (!hasStrength) {
          strength_ = 256;
        }
        break;
      case 2:
        radius_ = std::max(1, radius_);
        if (!hasStrength) {
          strength_ = 192;
        } else {
          strength_ = std::clamp(strength_, 0, 256);
        }
        break;
      case 3:
        radius_ = std::max(2, radius_);
        if (!hasStrength) {
          strength_ = 256;
        }
        break;
      default:
        break;
    }
  }

  horizontal_ = params.getBool("horizontal", horizontal_);
  vertical_ = params.getBool("vertical", vertical_);
}

void R_Blur::ensureBuffers(int width, int height) {
  const std::size_t total = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u;
  if (original_.size() < total) {
    original_.resize(total);
  }
  if (temp_.size() < total) {
    temp_.resize(total);
  }
  if (blurred_.size() < total) {
    blurred_.resize(total);
  }
  if (static_cast<int>(prefixRow_.size()) < (width + 1) * 4) {
    prefixRow_.assign(static_cast<std::size_t>(width + 1) * 4u, 0);
  }
  if (static_cast<int>(prefixColumn_.size()) < (height + 1) * 4) {
    prefixColumn_.assign(static_cast<std::size_t>(height + 1) * 4u, 0);
  }
}

bool R_Blur::renderBox(avs::core::RenderContext& context) {
  if (radius_ <= 0 || strength_ <= 0) {
    return true;
  }
  if (!horizontal_ && !vertical_) {
    return true;
  }

  const int width = std::max(0, context.width);
  const int height = std::max(0, context.height);
  if (width == 0 || height == 0) {
    return true;
  }

  ensureBuffers(width, height);
  std::uint8_t* framebuffer = context.framebuffer.data;
  const std::size_t total = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u;
  std::memcpy(original_.data(), framebuffer, total);

  std::uint8_t* firstPassDst = vertical_ ? temp_.data() : blurred_.data();
  if (horizontal_) {
    horizontalPass(original_.data(), firstPassDst, width, height);
  } else {
    std::memcpy(firstPassDst, original_.data(), total);
  }

  if (vertical_) {
    const std::uint8_t* verticalSrc = horizontal_ ? temp_.data() : original_.data();
    verticalPass(verticalSrc, blurred_.data(), width, height);
  }

  blend(framebuffer, original_.data(), blurred_.data(), width, height);
  return true;
}

void R_Blur::horizontalPass(const std::uint8_t* src,
                            std::uint8_t* dst,
                            int width,
                            int height) const {
  if (!horizontal_ || radius_ <= 0) {
    std::memcpy(dst, src, static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u);
    return;
  }

  const int window = radius_ * 2 + 1;
  const std::size_t stride = static_cast<std::size_t>(width) * 4u;
  for (int y = 0; y < height; ++y) {
    std::fill(prefixRow_.begin(), prefixRow_.begin() + static_cast<std::ptrdiff_t>((width + 1) * 4), 0);
    const std::uint8_t* row = src + static_cast<std::size_t>(y) * stride;
    for (int x = 0; x < width; ++x) {
      const std::size_t srcIndex = static_cast<std::size_t>(x) * 4u;
      const std::size_t prefixIndex = static_cast<std::size_t>(x + 1) * 4u;
      prefixRow_[prefixIndex + 0] = prefixRow_[prefixIndex - 4] + row[srcIndex + 0];
      prefixRow_[prefixIndex + 1] = prefixRow_[prefixIndex - 3] + row[srcIndex + 1];
      prefixRow_[prefixIndex + 2] = prefixRow_[prefixIndex - 2] + row[srcIndex + 2];
      prefixRow_[prefixIndex + 3] = prefixRow_[prefixIndex - 1] + row[srcIndex + 3];
    }
    std::uint8_t* dstRow = dst + static_cast<std::size_t>(y) * stride;
    const std::uint8_t* firstPx = row;
    const std::uint8_t* lastPx = row + static_cast<std::size_t>(width - 1) * 4u;
    for (int x = 0; x < width; ++x) {
      const int left = x - radius_;
      const int right = x + radius_;
      const int clampedLeft = clampIndex(left, 0, width - 1);
      const int clampedRight = clampIndex(right, 0, width - 1);
      const int leftPadding = clampedLeft - left;
      const int rightPadding = right - clampedRight;
      const std::size_t prefixLeft = static_cast<std::size_t>(clampedLeft) * 4u;
      const std::size_t prefixRight = static_cast<std::size_t>(clampedRight + 1) * 4u;
      std::uint8_t* dstPx = dstRow + static_cast<std::size_t>(x) * 4u;
      for (int channel = 0; channel < 4; ++channel) {
        int sum = prefixRow_[prefixRight + channel] - prefixRow_[prefixLeft + channel];
        if (leftPadding > 0) {
          sum += leftPadding * static_cast<int>(firstPx[channel]);
        }
        if (rightPadding > 0) {
          sum += rightPadding * static_cast<int>(lastPx[channel]);
        }
        const int rounding = roundMode_ ? window / 2 : 0;
        dstPx[channel] = clampByte((sum + rounding) / window);
      }
    }
  }
}

void R_Blur::verticalPass(const std::uint8_t* src,
                          std::uint8_t* dst,
                          int width,
                          int height) const {
  if (!vertical_ || radius_ <= 0) {
    std::memcpy(dst, src, static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u);
    return;
  }
  const int window = radius_ * 2 + 1;
  for (int x = 0; x < width; ++x) {
    std::fill(prefixColumn_.begin(),
              prefixColumn_.begin() + static_cast<std::ptrdiff_t>((height + 1) * 4),
              0);
    for (int y = 0; y < height; ++y) {
      const std::size_t srcIndex =
          (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)) * 4u;
      const std::size_t prefixIndex = static_cast<std::size_t>(y + 1) * 4u;
      prefixColumn_[prefixIndex + 0] = prefixColumn_[prefixIndex - 4] + src[srcIndex + 0];
      prefixColumn_[prefixIndex + 1] = prefixColumn_[prefixIndex - 3] + src[srcIndex + 1];
      prefixColumn_[prefixIndex + 2] = prefixColumn_[prefixIndex - 2] + src[srcIndex + 2];
      prefixColumn_[prefixIndex + 3] = prefixColumn_[prefixIndex - 1] + src[srcIndex + 3];
    }
    const std::uint8_t* firstPx = src + static_cast<std::size_t>(x) * 4u;
    const std::uint8_t* lastPx =
        src + (static_cast<std::size_t>(height - 1) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)) * 4u;
    for (int y = 0; y < height; ++y) {
      const int top = y - radius_;
      const int bottom = y + radius_;
      const int clampedTop = clampIndex(top, 0, height - 1);
      const int clampedBottom = clampIndex(bottom, 0, height - 1);
      const int topPadding = clampedTop - top;
      const int bottomPadding = bottom - clampedBottom;
      const std::size_t prefixTop = static_cast<std::size_t>(clampedTop) * 4u;
      const std::size_t prefixBottom = static_cast<std::size_t>(clampedBottom + 1) * 4u;
      std::uint8_t* dstPx = dst +
                             (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                              static_cast<std::size_t>(x)) *
                                 4u;
      for (int channel = 0; channel < 4; ++channel) {
        int sum = prefixColumn_[prefixBottom + channel] - prefixColumn_[prefixTop + channel];
        if (topPadding > 0) {
          sum += topPadding * static_cast<int>(firstPx[channel]);
        }
        if (bottomPadding > 0) {
          sum += bottomPadding * static_cast<int>(lastPx[channel]);
        }
        const int rounding = roundMode_ ? window / 2 : 0;
        dstPx[channel] = clampByte((sum + rounding) / window);
      }
    }
  }
}

void R_Blur::blend(std::uint8_t* dst,
                   const std::uint8_t* original,
                   const std::uint8_t* blurred,
                   int width,
                   int height) const {
  const std::size_t total = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u;
  if (strength_ >= 256) {
    std::memcpy(dst, blurred, total);
    return;
  }
  if (strength_ <= 0) {
    std::memcpy(dst, original, total);
    return;
  }
  const int invStrength = 256 - strength_;
  const int rounding = roundMode_ ? 128 : 0;
  for (std::size_t i = 0; i < total; ++i) {
    const int value = static_cast<int>(blurred[i]) * strength_ + static_cast<int>(original[i]) * invStrength + rounding;
    dst[i] = clampByte(value >> 8);
  }
}

int R_Blur::clampIndex(int value, int minValue, int maxValue) {
  if (value < minValue) {
    return minValue;
  }
  if (value > maxValue) {
    return maxValue;
  }
  return value;
}

std::uint8_t R_Blur::clampByte(int value) {
  if (value <= 0) {
    return 0u;
  }
  if (value >= 255) {
    return 255u;
  }
  return static_cast<std::uint8_t>(value);
}

}  // namespace avs::effects::trans

