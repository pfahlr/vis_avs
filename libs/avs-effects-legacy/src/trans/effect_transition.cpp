#include "avs/effects/trans/effect_transition.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace avs::effects::trans {

namespace {
constexpr double kDefaultTransitionDuration = 0.25;  // 250ms in seconds
constexpr double kPi = 3.14159265358979323846;
}  // namespace

TransitionEffect::TransitionEffect() = default;

void TransitionEffect::setParams(const avs::core::ParamBlock& params) {
  if (params.contains("mode")) {
    auto modeValue = params.getInt("mode", static_cast<int>(mode_));
    if (modeValue >= 0 && modeValue < static_cast<int>(kModeCount)) {
      mode_ = static_cast<Mode>(modeValue);
    }
  }
  if (params.contains("speed")) {
    transitionSpeed_ = std::max(0.1f, params.getFloat("speed", transitionSpeed_));
  }
  if (params.contains("enabled")) {
    setEnabled(params.getBool("enabled", enabled_));
  }
}

void TransitionEffect::setEnabled(bool enabled) {
  if (enabled && !enabled_) {
    // Transition is being enabled - reset state
    transitionStartTime_ = 0.0;
    blockMask_ = 0;
  }
  enabled_ = enabled;
}

bool TransitionEffect::render(avs::core::RenderContext& context) {
  if (!enabled_) {
    // Pass through - no transition
    return true;
  }

  prepareBuffers(context);

  if (!buffersValid_) {
    return true;
  }

  // Calculate transition progress [0, 1]
  if (transitionStartTime_ == 0.0) {
    transitionStartTime_ = context.frameIndex * context.deltaSeconds;
  }

  const double elapsed = (context.frameIndex * context.deltaSeconds) - transitionStartTime_;
  const double duration = kDefaultTransitionDuration / transitionSpeed_;
  float progress = static_cast<float>(elapsed / duration);

  if (progress >= 1.0f) {
    // Transition complete - swap buffers and restart
    bufferA_.swap(bufferB_);
    transitionStartTime_ = context.frameIndex * context.deltaSeconds;
    progress = 0.0f;
    blockMask_ = 0;
  }

  // Apply smooth easing curve
  const float t = smoothCurve(progress);

  // Render the appropriate transition mode
  switch (mode_) {
    case Mode::kRandom:
      // Pick a random mode (excluding Random itself)
      renderCrossDissolve(context, t);
      break;
    case Mode::kCrossDissolve:
      renderCrossDissolve(context, t);
      break;
    case Mode::kLeftRightPush:
      renderLeftRightPush(context, t);
      break;
    case Mode::kRightLeftPush:
      renderRightLeftPush(context, t);
      break;
    case Mode::kTopBottomPush:
      renderTopBottomPush(context, t);
      break;
    case Mode::kBottomTopPush:
      renderBottomTopPush(context, t);
      break;
    case Mode::kNineRandomBlocks:
      renderNineRandomBlocks(context, t);
      break;
    case Mode::kSplitLeftRightPush:
      renderSplitLeftRightPush(context, t);
      break;
    case Mode::kLeftRightToCenterPush:
      renderLeftRightToCenterPush(context, t);
      break;
    case Mode::kLeftRightToCenterSqueeze:
      renderLeftRightToCenterSqueeze(context, t);
      break;
    case Mode::kLeftRightWipe:
      renderLeftRightWipe(context, t);
      break;
    case Mode::kRightLeftWipe:
      renderRightLeftWipe(context, t);
      break;
    case Mode::kTopBottomWipe:
      renderTopBottomWipe(context, t);
      break;
    case Mode::kBottomTopWipe:
      renderBottomTopWipe(context, t);
      break;
    case Mode::kDotDissolve:
      renderDotDissolve(context, t);
      break;
  }

  // Store current frame as buffer B for next transition
  std::memcpy(bufferB_.data(), context.framebuffer.data,
              bufferWidth_ * bufferHeight_ * 4);

  return true;
}

void TransitionEffect::prepareBuffers(const avs::core::RenderContext& context) {
  const std::size_t requiredSize = context.width * context.height * 4;

  if (bufferWidth_ != context.width || bufferHeight_ != context.height) {
    bufferWidth_ = context.width;
    bufferHeight_ = context.height;
    bufferA_.resize(requiredSize);
    bufferB_.resize(requiredSize);

    // Initialize with current frame
    std::memcpy(bufferA_.data(), context.framebuffer.data, requiredSize);
    std::memcpy(bufferB_.data(), context.framebuffer.data, requiredSize);
    buffersValid_ = true;
  }
}

float TransitionEffect::smoothCurve(float t) {
  // Smooth S-curve using sine: sin((t - 0.5) * PI) / 2 + 0.5
  return static_cast<float>(std::sin((t - 0.5) * kPi) * 0.5 + 0.5);
}

std::array<std::uint8_t, 4> TransitionEffect::blendColors(
    const std::array<std::uint8_t, 4>& a,
    const std::array<std::uint8_t, 4>& b,
    float t) {
  const float oneMinusT = 1.0f - t;
  return {
      static_cast<std::uint8_t>(a[0] * oneMinusT + b[0] * t),
      static_cast<std::uint8_t>(a[1] * oneMinusT + b[1] * t),
      static_cast<std::uint8_t>(a[2] * oneMinusT + b[2] * t),
      static_cast<std::uint8_t>(a[3] * oneMinusT + b[3] * t),
  };
}

void TransitionEffect::renderCrossDissolve(avs::core::RenderContext& context, float t) {
  const std::size_t pixelCount = bufferWidth_ * bufferHeight_;
  const std::uint8_t* srcA = bufferA_.data();
  const std::uint8_t* srcB = bufferB_.data();
  std::uint8_t* dst = context.framebuffer.data;

  for (std::size_t i = 0; i < pixelCount; ++i) {
    const std::size_t offset = i * 4;
    const std::array<std::uint8_t, 4> colorA = {
        srcA[offset], srcA[offset + 1], srcA[offset + 2], srcA[offset + 3]};
    const std::array<std::uint8_t, 4> colorB = {
        srcB[offset], srcB[offset + 1], srcB[offset + 2], srcB[offset + 3]};
    const auto blended = blendColors(colorA, colorB, t);
    dst[offset] = blended[0];
    dst[offset + 1] = blended[1];
    dst[offset + 2] = blended[2];
    dst[offset + 3] = blended[3];
  }
}

void TransitionEffect::renderLeftRightPush(avs::core::RenderContext& context, float t) {
  const int pushOffset = static_cast<int>(t * bufferWidth_);
  const std::uint8_t* srcA = bufferA_.data();
  const std::uint8_t* srcB = bufferB_.data();
  std::uint8_t* dst = context.framebuffer.data;

  for (int y = 0; y < bufferHeight_; ++y) {
    const int rowOffset = y * bufferWidth_ * 4;
    // Copy pushed part of A (from right side)
    const int srcAOffset = rowOffset + (bufferWidth_ - pushOffset) * 4;
    std::memcpy(dst + rowOffset, srcA + srcAOffset, pushOffset * 4);
    // Copy revealed part of B
    const int dstOffset = rowOffset + pushOffset * 4;
    std::memcpy(dst + dstOffset, srcB + rowOffset, (bufferWidth_ - pushOffset) * 4);
  }
}

void TransitionEffect::renderRightLeftPush(avs::core::RenderContext& context, float t) {
  const int pushOffset = static_cast<int>(t * bufferWidth_);
  const std::uint8_t* srcA = bufferA_.data();
  const std::uint8_t* srcB = bufferB_.data();
  std::uint8_t* dst = context.framebuffer.data;

  for (int y = 0; y < bufferHeight_; ++y) {
    const int rowOffset = y * bufferWidth_ * 4;
    // Copy revealed part of B
    const int srcBOffset = rowOffset + pushOffset * 4;
    std::memcpy(dst + rowOffset, srcB + srcBOffset, (bufferWidth_ - pushOffset) * 4);
    // Copy pushed part of A
    const int dstOffset = rowOffset + (bufferWidth_ - pushOffset) * 4;
    std::memcpy(dst + dstOffset, srcA + rowOffset, pushOffset * 4);
  }
}

void TransitionEffect::renderTopBottomPush(avs::core::RenderContext& context, float t) {
  const int pushOffset = static_cast<int>(t * bufferHeight_);
  const std::uint8_t* srcA = bufferA_.data();
  const std::uint8_t* srcB = bufferB_.data();
  std::uint8_t* dst = context.framebuffer.data;

  // Copy pushed part of A (from bottom)
  const std::size_t pushBytes = pushOffset * bufferWidth_ * 4;
  const std::size_t srcAOffset = (bufferHeight_ - pushOffset) * bufferWidth_ * 4;
  std::memcpy(dst, srcA + srcAOffset, pushBytes);

  // Copy revealed part of B
  const std::size_t revealBytes = (bufferHeight_ - pushOffset) * bufferWidth_ * 4;
  std::memcpy(dst + pushBytes, srcB, revealBytes);
}

void TransitionEffect::renderBottomTopPush(avs::core::RenderContext& context, float t) {
  const int pushOffset = static_cast<int>(t * bufferHeight_);
  const std::uint8_t* srcA = bufferA_.data();
  const std::uint8_t* srcB = bufferB_.data();
  std::uint8_t* dst = context.framebuffer.data;

  // Copy revealed part of B
  const std::size_t revealBytes = (bufferHeight_ - pushOffset) * bufferWidth_ * 4;
  const std::size_t srcBOffset = pushOffset * bufferWidth_ * 4;
  std::memcpy(dst, srcB + srcBOffset, revealBytes);

  // Copy pushed part of A
  std::memcpy(dst + revealBytes, srcA, pushOffset * bufferWidth_ * 4);
}

void TransitionEffect::renderNineRandomBlocks(avs::core::RenderContext& context, float t) {
  // Divide screen into 3x3 grid and reveal blocks randomly
  const int stepCount = 9;
  const int currentStep = static_cast<int>(t * stepCount);

  // Check if we need to add a new block
  const int stepMask = 1 << (10 + currentStep / (stepCount / 9));
  if ((blockMask_ & stepMask) == 0 && currentStep < stepCount) {
    // Pick a random unset block
    int block;
    do {
      block = static_cast<int>(context.rng.nextUint32() % 9);
    } while ((blockMask_ & (1 << block)) != 0 && blockMask_ != 0x1FF);
    blockMask_ |= (1 << block) | stepMask;
  }

  const int blockWidth = bufferWidth_ / 3;
  const int blockHeight = bufferHeight_ / 3;
  const int blockWidthRem = bufferWidth_ - 2 * blockWidth;
  const int blockHeightRem = bufferHeight_ - 2 * blockHeight;

  const std::uint8_t* srcA = bufferA_.data();
  const std::uint8_t* srcB = bufferB_.data();
  std::uint8_t* dst = context.framebuffer.data;

  // Start with buffer B
  std::memcpy(dst, srcB, bufferWidth_ * bufferHeight_ * 4);

  // Overwrite revealed blocks with buffer A
  for (int block = 0; block < 9; ++block) {
    if ((blockMask_ & (1 << block)) == 0) {
      continue;
    }

    const int bx = block % 3;
    const int by = block / 3;
    const int blockW = (bx == 2) ? blockWidthRem : blockWidth;
    const int blockH = (by == 2) ? blockHeightRem : blockHeight;

    for (int y = 0; y < blockH; ++y) {
      const int srcY = by * blockHeight + y;
      const int srcX = bx * blockWidth;
      const std::size_t srcOffset = (srcY * bufferWidth_ + srcX) * 4;
      const std::size_t dstOffset = srcOffset;
      std::memcpy(dst + dstOffset, srcA + srcOffset, blockW * 4);
    }
  }
}

void TransitionEffect::renderSplitLeftRightPush(avs::core::RenderContext& context, float t) {
  const int pushOffset = static_cast<int>(t * bufferWidth_);
  const std::uint8_t* srcA = bufferA_.data();
  const std::uint8_t* srcB = bufferB_.data();
  std::uint8_t* dst = context.framebuffer.data;

  const int halfHeight = bufferHeight_ / 2;

  // Top half: push left to right
  for (int y = 0; y < halfHeight; ++y) {
    const int rowOffset = y * bufferWidth_ * 4;
    // Revealed part of B
    std::memcpy(dst + rowOffset, srcB + rowOffset + pushOffset * 4,
                (bufferWidth_ - pushOffset) * 4);
    // Pushed part of A (from right side)
    const int dstOffset = rowOffset + (bufferWidth_ - pushOffset) * 4;
    const int srcAOffset = rowOffset + (bufferWidth_ - pushOffset) * 4;
    std::memcpy(dst + dstOffset, srcA + srcAOffset, pushOffset * 4);
  }

  // Bottom half: push right to left
  for (int y = halfHeight; y < bufferHeight_; ++y) {
    const int rowOffset = y * bufferWidth_ * 4;
    // Revealed part of B
    const int srcBOffset = rowOffset + pushOffset * 4;
    std::memcpy(dst + rowOffset, srcB + srcBOffset, (bufferWidth_ - pushOffset) * 4);
    // Pushed part of A
    const int dstOffset = rowOffset + (bufferWidth_ - pushOffset) * 4;
    std::memcpy(dst + dstOffset, srcA + rowOffset, pushOffset * 4);
  }
}

void TransitionEffect::renderLeftRightToCenterPush(avs::core::RenderContext& context,
                                                    float t) {
  const int pushOffset = static_cast<int>(t * bufferWidth_ / 2);
  const std::uint8_t* srcA = bufferA_.data();
  const std::uint8_t* srcB = bufferB_.data();
  std::uint8_t* dst = context.framebuffer.data;

  for (int y = 0; y < bufferHeight_; ++y) {
    const int rowOffset = y * bufferWidth_ * 4;
    // Left side: push from left
    const int srcALeftOffset = rowOffset + (bufferWidth_ / 2 - pushOffset) * 4;
    std::memcpy(dst + rowOffset, srcA + srcALeftOffset, pushOffset * 4);
    // Right side: push from right
    const int dstRightOffset = rowOffset + (bufferWidth_ - pushOffset) * 4;
    const int srcARightOffset = rowOffset + bufferWidth_ / 2 * 4;
    std::memcpy(dst + dstRightOffset, srcA + srcARightOffset, pushOffset * 4);
    // Center: revealed part of B
    const int centerOffset = rowOffset + pushOffset * 4;
    const int centerWidth = bufferWidth_ - 2 * pushOffset;
    std::memcpy(dst + centerOffset, srcB + centerOffset, centerWidth * 4);
  }
}

void TransitionEffect::renderLeftRightToCenterSqueeze(avs::core::RenderContext& context,
                                                       float t) {
  const int squeezeWidth = static_cast<int>(t * bufferWidth_ / 2);
  const std::uint8_t* srcA = bufferA_.data();
  const std::uint8_t* srcB = bufferB_.data();
  std::uint8_t* dst = context.framebuffer.data;

  for (int y = 0; y < bufferHeight_; ++y) {
    const int rowOffset = y * bufferWidth_ * 4;

    // Left squeezed section
    if (squeezeWidth > 0) {
      const float scaleX = (bufferWidth_ / 2.0f) / squeezeWidth;
      for (int x = 0; x < squeezeWidth; ++x) {
        const int srcX = static_cast<int>(x * scaleX);
        const std::size_t srcOffset = (y * bufferWidth_ + srcX) * 4;
        const std::size_t dstOffset = rowOffset + x * 4;
        std::memcpy(dst + dstOffset, srcA + srcOffset, 4);
      }
    }

    // Center section from B
    const int centerWidth = bufferWidth_ - 2 * squeezeWidth;
    if (centerWidth > 0) {
      const float scaleX = static_cast<float>(bufferWidth_) / centerWidth;
      for (int x = 0; x < centerWidth; ++x) {
        const int srcX = static_cast<int>(x * scaleX);
        const std::size_t srcOffset = (y * bufferWidth_ + srcX) * 4;
        const std::size_t dstOffset = rowOffset + (squeezeWidth + x) * 4;
        std::memcpy(dst + dstOffset, srcB + srcOffset, 4);
      }
    }

    // Right squeezed section
    if (squeezeWidth > 0) {
      const float scaleX = (bufferWidth_ / 2.0f) / squeezeWidth;
      for (int x = 0; x < squeezeWidth; ++x) {
        const int srcX = bufferWidth_ / 2 + static_cast<int>(x * scaleX);
        const std::size_t srcOffset = (y * bufferWidth_ + srcX) * 4;
        const std::size_t dstOffset = rowOffset + (bufferWidth_ - squeezeWidth + x) * 4;
        std::memcpy(dst + dstOffset, srcA + srcOffset, 4);
      }
    }
  }
}

void TransitionEffect::renderLeftRightWipe(avs::core::RenderContext& context, float t) {
  const int wipeOffset = static_cast<int>(t * bufferWidth_);
  const std::uint8_t* srcA = bufferA_.data();
  const std::uint8_t* srcB = bufferB_.data();
  std::uint8_t* dst = context.framebuffer.data;

  for (int y = 0; y < bufferHeight_; ++y) {
    const int rowOffset = y * bufferWidth_ * 4;
    // Wiped part (A)
    std::memcpy(dst + rowOffset, srcA + rowOffset, wipeOffset * 4);
    // Remaining part (B)
    const int remainOffset = rowOffset + wipeOffset * 4;
    std::memcpy(dst + remainOffset, srcB + remainOffset, (bufferWidth_ - wipeOffset) * 4);
  }
}

void TransitionEffect::renderRightLeftWipe(avs::core::RenderContext& context, float t) {
  const int wipeOffset = static_cast<int>(t * bufferWidth_);
  const std::uint8_t* srcA = bufferA_.data();
  const std::uint8_t* srcB = bufferB_.data();
  std::uint8_t* dst = context.framebuffer.data;

  for (int y = 0; y < bufferHeight_; ++y) {
    const int rowOffset = y * bufferWidth_ * 4;
    // Remaining part (B)
    std::memcpy(dst + rowOffset, srcB + rowOffset, (bufferWidth_ - wipeOffset) * 4);
    // Wiped part (A)
    const int wipeStart = rowOffset + (bufferWidth_ - wipeOffset) * 4;
    std::memcpy(dst + wipeStart, srcA + wipeStart, wipeOffset * 4);
  }
}

void TransitionEffect::renderTopBottomWipe(avs::core::RenderContext& context, float t) {
  const int wipeOffset = static_cast<int>(t * bufferHeight_);
  const std::uint8_t* srcA = bufferA_.data();
  const std::uint8_t* srcB = bufferB_.data();
  std::uint8_t* dst = context.framebuffer.data;

  const std::size_t wipeBytes = wipeOffset * bufferWidth_ * 4;
  // Wiped part (A)
  std::memcpy(dst, srcA, wipeBytes);
  // Remaining part (B)
  std::memcpy(dst + wipeBytes, srcB + wipeBytes,
              (bufferHeight_ - wipeOffset) * bufferWidth_ * 4);
}

void TransitionEffect::renderBottomTopWipe(avs::core::RenderContext& context, float t) {
  const int wipeOffset = static_cast<int>(t * bufferHeight_);
  const std::uint8_t* srcA = bufferA_.data();
  const std::uint8_t* srcB = bufferB_.data();
  std::uint8_t* dst = context.framebuffer.data;

  const std::size_t remainBytes = (bufferHeight_ - wipeOffset) * bufferWidth_ * 4;
  // Remaining part (B)
  std::memcpy(dst, srcB, remainBytes);
  // Wiped part (A)
  std::memcpy(dst + remainBytes, srcA + remainBytes, wipeOffset * bufferWidth_ * 4);
}

void TransitionEffect::renderDotDissolve(avs::core::RenderContext& context, float t) {
  // Checkerboard pattern that gets finer as transition progresses
  const int dotSize = std::max(1, static_cast<int>((1.0f - t) * 32));
  const std::uint8_t* srcA = bufferA_.data();
  const std::uint8_t* srcB = bufferB_.data();
  std::uint8_t* dst = context.framebuffer.data;

  for (int y = 0; y < bufferHeight_; ++y) {
    const bool rowSwitch = ((y / dotSize) & 1) == 0;
    for (int x = 0; x < bufferWidth_; ++x) {
      const bool colSwitch = ((x / dotSize) & 1) == 0;
      const bool useA = rowSwitch ^ colSwitch;
      const std::size_t offset = (y * bufferWidth_ + x) * 4;
      const std::uint8_t* src = useA ? srcA : srcB;
      std::memcpy(dst + offset, src + offset, 4);
    }
  }
}

}  // namespace avs::effects::trans
