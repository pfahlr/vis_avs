#include <avs/effects/filters/effect_blur_box.h>

#include <algorithm>
#include <cstring>

#include <avs/effects/filters/filter_common.h>

namespace avs::effects::filters {

namespace {
constexpr int kMaxRadius = 32;
}

void BlurBox::setParams(const avs::core::ParamBlock& params) {
  radius_ = std::clamp(params.getInt("radius", radius_), 0, kMaxRadius);
  preserveAlpha_ = params.getBool("preserve_alpha", true);
}

void BlurBox::ensureBuffers(int width, int height) {
  const std::size_t required = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u;
  ensureScratch(scratch_, required);
  if (static_cast<int>(prefixRow_.size()) < (width + 1) * 4) {
    prefixRow_.assign(static_cast<std::size_t>(width + 1) * 4u, 0);
  }
  if (static_cast<int>(prefixColumn_.size()) < (height + 1) * 4) {
    prefixColumn_.assign(static_cast<std::size_t>(height + 1) * 4u, 0);
  }
}

void BlurBox::horizontalPass(const std::uint8_t* src, std::uint8_t* dst, int width, int height) const {
  if (radius_ <= 0) {
    std::memcpy(dst, src, static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u);
    return;
  }
  const int window = radius_ * 2 + 1;
  const int stride = width * 4;
  for (int y = 0; y < height; ++y) {
    std::fill(prefixRow_.begin(), prefixRow_.begin() + static_cast<std::ptrdiff_t>((width + 1) * 4), 0);
    const std::uint8_t* row = src + static_cast<std::size_t>(y) * stride;
    std::uint8_t* dstRow = dst + static_cast<std::size_t>(y) * stride;
    for (int x = 0; x < width; ++x) {
      const std::size_t srcIndex = static_cast<std::size_t>(x) * 4u;
      const std::size_t prefixIndex = static_cast<std::size_t>(x + 1) * 4u;
      prefixRow_[prefixIndex + 0] = prefixRow_[prefixIndex - 4] + row[srcIndex + 0];
      prefixRow_[prefixIndex + 1] = prefixRow_[prefixIndex - 3] + row[srcIndex + 1];
      prefixRow_[prefixIndex + 2] = prefixRow_[prefixIndex - 2] + row[srcIndex + 2];
      prefixRow_[prefixIndex + 3] = prefixRow_[prefixIndex - 1] + row[srcIndex + 3];
    }
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
      const std::uint8_t* firstPx = row;
      const std::uint8_t* lastPx = row + static_cast<std::size_t>(width - 1) * 4u;
      for (int channel = 0; channel < 4; ++channel) {
        int sum = prefixRow_[prefixRight + channel] - prefixRow_[prefixLeft + channel];
        if (leftPadding > 0) {
          sum += leftPadding * static_cast<int>(firstPx[channel]);
        }
        if (rightPadding > 0) {
          sum += rightPadding * static_cast<int>(lastPx[channel]);
        }
        if (preserveAlpha_ && channel == 3) {
          dstPx[channel] = row[static_cast<std::size_t>(x) * 4u + channel];
        } else {
          dstPx[channel] = clampByte((sum + window / 2) / window);
        }
      }
    }
  }
}

void BlurBox::verticalPass(const std::uint8_t* src, std::uint8_t* dst, int width, int height) const {
  if (radius_ <= 0) {
    std::memcpy(dst, src, static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u);
    return;
  }
  const int window = radius_ * 2 + 1;
  for (int x = 0; x < width; ++x) {
    std::fill(prefixColumn_.begin(), prefixColumn_.begin() + static_cast<std::ptrdiff_t>((height + 1) * 4), 0);
    for (int y = 0; y < height; ++y) {
      const std::size_t srcIndex = (static_cast<std::size_t>(y) * width + static_cast<std::size_t>(x)) * 4u;
      const std::size_t prefixIndex = static_cast<std::size_t>(y + 1) * 4u;
      prefixColumn_[prefixIndex + 0] = prefixColumn_[prefixIndex - 4] + src[srcIndex + 0];
      prefixColumn_[prefixIndex + 1] = prefixColumn_[prefixIndex - 3] + src[srcIndex + 1];
      prefixColumn_[prefixIndex + 2] = prefixColumn_[prefixIndex - 2] + src[srcIndex + 2];
      prefixColumn_[prefixIndex + 3] = prefixColumn_[prefixIndex - 1] + src[srcIndex + 3];
    }
    const std::uint8_t* firstPx = src + static_cast<std::size_t>(x) * 4u;
    const std::uint8_t* lastPx = src + (static_cast<std::size_t>(height - 1) * width + static_cast<std::size_t>(x)) * 4u;
    for (int y = 0; y < height; ++y) {
      const int top = y - radius_;
      const int bottom = y + radius_;
      const int clampedTop = clampIndex(top, 0, height - 1);
      const int clampedBottom = clampIndex(bottom, 0, height - 1);
      const int topPadding = clampedTop - top;
      const int bottomPadding = bottom - clampedBottom;
      const std::size_t prefixTop = static_cast<std::size_t>(clampedTop) * 4u;
      const std::size_t prefixBottom = static_cast<std::size_t>(clampedBottom + 1) * 4u;
      std::uint8_t* dstPx = dst + (static_cast<std::size_t>(y) * width + static_cast<std::size_t>(x)) * 4u;
      for (int channel = 0; channel < 4; ++channel) {
        int sum = prefixColumn_[prefixBottom + channel] - prefixColumn_[prefixTop + channel];
        if (topPadding > 0) {
          sum += topPadding * static_cast<int>(firstPx[channel]);
        }
        if (bottomPadding > 0) {
          sum += bottomPadding * static_cast<int>(lastPx[channel]);
        }
        if (preserveAlpha_ && channel == 3) {
          dstPx[channel] = src[(static_cast<std::size_t>(y) * width + static_cast<std::size_t>(x)) * 4u + channel];
        } else {
          dstPx[channel] = clampByte((sum + window / 2) / window);
        }
      }
    }
  }
}

bool BlurBox::render(avs::core::RenderContext& context) {
  if (!hasFramebuffer(context)) {
    return true;
  }
  const int width = std::max(0, context.width);
  const int height = std::max(0, context.height);
  if (radius_ <= 0) {
    return true;
  }
  ensureBuffers(width, height);

  std::uint8_t* framebuffer = context.framebuffer.data;
  horizontalPass(framebuffer, scratch_.data(), width, height);
  verticalPass(scratch_.data(), framebuffer, width, height);

  if (preserveAlpha_) {
    const std::size_t pixels = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    for (std::size_t i = 0; i < pixels; ++i) {
      framebuffer[i * 4u + 3] = scratch_[i * 4u + 3];
    }
  }
  return true;
}

}  // namespace avs::effects::filters

