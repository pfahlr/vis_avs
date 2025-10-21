#include "effects/render/effect_avi.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <string>

#include "stb_image.h"

namespace {
constexpr const char* kLogPrefix = "[Render / AVI]";

std::string failureReason() {
  const char* reason = stbi_failure_reason();
  return reason != nullptr ? std::string(reason) : std::string();
}
}  // namespace

Effect_RenderAvi::Effect_RenderAvi() {
  std::clog << kLogPrefix << " AVI playback is not yet implemented; falling back to static imagery."
            << std::endl;
}

void Effect_RenderAvi::setParams(const avs::core::ParamBlock& params) {
  const std::string newVideoPath =
      params.getString("file", params.getString("source", params.getString("path", videoPath_)));
  if (newVideoPath != videoPath_) {
    videoPath_ = newVideoPath;
    warnedUnsupported_ = false;
  }

  const std::string newFallbackPath = params.getString(
      "fallback", params.getString("thumbnail", params.getString("poster", fallbackImagePath_)));
  if (newFallbackPath != fallbackImagePath_) {
    fallbackImagePath_ = newFallbackPath;
    fallbackImage_ = ImageData{};
    fallbackLoadAttempted_ = false;
  }
}

bool Effect_RenderAvi::render(avs::core::RenderContext& context) {
  if (!hasFramebuffer(context)) {
    return true;
  }

  if (!videoPath_.empty() && !warnedUnsupported_) {
    std::clog << kLogPrefix << " Unable to render AVI '" << videoPath_
              << "' — decoder not available. Using fallback." << std::endl;
    warnedUnsupported_ = true;
  }

  if (ensureFallbackImageLoaded()) {
    drawPlaceholder(context);
    drawFallbackImage(context);
  } else {
    drawPlaceholder(context);
  }

  return true;
}

bool Effect_RenderAvi::hasFramebuffer(const avs::core::RenderContext& context) const {
  if (!context.framebuffer.data || context.width <= 0 || context.height <= 0) {
    return false;
  }
  const std::size_t expected =
      static_cast<std::size_t>(context.width) * static_cast<std::size_t>(context.height) * 4u;
  return context.framebuffer.size >= expected;
}

bool Effect_RenderAvi::ensureFallbackImageLoaded() {
  if (!fallbackImage_.pixels.empty() && fallbackImage_.width > 0 && fallbackImage_.height > 0) {
    return true;
  }
  if (fallbackLoadAttempted_ || fallbackImagePath_.empty()) {
    return false;
  }
  fallbackLoadAttempted_ = true;

  int width = 0;
  int height = 0;
  int channels = 0;
  stbi_uc* decoded =
      stbi_load(fallbackImagePath_.c_str(), &width, &height, &channels, STBI_rgb_alpha);
  if (!decoded) {
    std::clog << kLogPrefix << " Failed to load fallback image '" << fallbackImagePath_
              << "': " << failureReason() << std::endl;
    return false;
  }

  const std::size_t pixelCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
  fallbackImage_.width = width;
  fallbackImage_.height = height;
  fallbackImage_.pixels.assign(decoded, decoded + pixelCount * 4u);
  stbi_image_free(decoded);
  std::clog << kLogPrefix << " Loaded fallback image '" << fallbackImagePath_ << "' (" << width
            << "x" << height << ")." << std::endl;
  return true;
}

void Effect_RenderAvi::drawFallbackImage(avs::core::RenderContext& context) const {
  if (fallbackImage_.pixels.empty() || fallbackImage_.width <= 0 || fallbackImage_.height <= 0) {
    return;
  }

  const int fbWidth = context.width;
  const int fbHeight = context.height;
  const float scaleFit =
      std::min(static_cast<float>(fbWidth) / static_cast<float>(fallbackImage_.width),
               static_cast<float>(fbHeight) / static_cast<float>(fallbackImage_.height));
  const float scale = std::min(scaleFit, 1.0f);
  const float invScale = scale > 0.0f ? 1.0f / scale : 0.0f;
  const int targetWidth =
      static_cast<int>(std::round(static_cast<float>(fallbackImage_.width) * scale));
  const int targetHeight =
      static_cast<int>(std::round(static_cast<float>(fallbackImage_.height) * scale));
  const int offsetX = (fbWidth - targetWidth) / 2;
  const int offsetY = (fbHeight - targetHeight) / 2;

  std::uint8_t* dst = context.framebuffer.data;
  for (int y = 0; y < targetHeight; ++y) {
    const int destY = offsetY + y;
    if (destY < 0 || destY >= fbHeight) {
      continue;
    }
    const int srcY = std::clamp(static_cast<int>(std::floor(static_cast<float>(y) * invScale)), 0,
                                fallbackImage_.height - 1);
    for (int x = 0; x < targetWidth; ++x) {
      const int destX = offsetX + x;
      if (destX < 0 || destX >= fbWidth) {
        continue;
      }
      const int srcX = std::clamp(static_cast<int>(std::floor(static_cast<float>(x) * invScale)), 0,
                                  fallbackImage_.width - 1);
      const std::size_t srcIndex =
          static_cast<std::size_t>(srcY) * static_cast<std::size_t>(fallbackImage_.width) * 4u +
          static_cast<std::size_t>(srcX) * 4u;
      const std::size_t dstIndex =
          static_cast<std::size_t>(destY) * static_cast<std::size_t>(fbWidth) * 4u +
          static_cast<std::size_t>(destX) * 4u;
      if (dstIndex + 3u >= context.framebuffer.size ||
          srcIndex + 3u >= fallbackImage_.pixels.size()) {
        continue;
      }
      dst[dstIndex + 0u] = fallbackImage_.pixels[srcIndex + 0u];
      dst[dstIndex + 1u] = fallbackImage_.pixels[srcIndex + 1u];
      dst[dstIndex + 2u] = fallbackImage_.pixels[srcIndex + 2u];
      dst[dstIndex + 3u] = fallbackImage_.pixels[srcIndex + 3u];
    }
  }
}

void Effect_RenderAvi::drawPlaceholder(avs::core::RenderContext& context) const {
  const int fbWidth = context.width;
  const int fbHeight = context.height;
  std::uint8_t* dst = context.framebuffer.data;
  const std::size_t stride = static_cast<std::size_t>(fbWidth) * 4u;
  for (int y = 0; y < fbHeight; ++y) {
    for (int x = 0; x < fbWidth; ++x) {
      const bool checker = ((x / 8) + (y / 8)) % 2 == 0;
      const std::uint8_t base =
          checker ? static_cast<std::uint8_t>(0x28) : static_cast<std::uint8_t>(0x18);
      const std::size_t index =
          static_cast<std::size_t>(y) * stride + static_cast<std::size_t>(x) * 4u;
      if (index + 3u >= context.framebuffer.size) {
        continue;
      }
      dst[index + 0u] = base;
      dst[index + 1u] = base;
      dst[index + 2u] = base;
      dst[index + 3u] = 255u;
    }
  }

  // Draw a subtle border to hint at media frame boundaries.
  if (fbWidth <= 0 || fbHeight <= 0) {
    return;
  }
  const std::uint8_t borderColor = 0x55;
  for (int x = 0; x < fbWidth; ++x) {
    const std::size_t topIndex = static_cast<std::size_t>(x) * 4u;
    const std::size_t bottomIndex =
        static_cast<std::size_t>(fbHeight - 1) * stride + static_cast<std::size_t>(x) * 4u;
    if (topIndex + 3u < context.framebuffer.size) {
      dst[topIndex + 0u] = borderColor;
      dst[topIndex + 1u] = borderColor;
      dst[topIndex + 2u] = borderColor;
      dst[topIndex + 3u] = 255u;
    }
    if (bottomIndex + 3u < context.framebuffer.size) {
      dst[bottomIndex + 0u] = borderColor;
      dst[bottomIndex + 1u] = borderColor;
      dst[bottomIndex + 2u] = borderColor;
      dst[bottomIndex + 3u] = 255u;
    }
  }
  for (int y = 0; y < fbHeight; ++y) {
    const std::size_t leftIndex = static_cast<std::size_t>(y) * stride;
    const std::size_t rightIndex =
        static_cast<std::size_t>(y) * stride + static_cast<std::size_t>(fbWidth - 1) * 4u;
    if (leftIndex + 3u < context.framebuffer.size) {
      dst[leftIndex + 0u] = borderColor;
      dst[leftIndex + 1u] = borderColor;
      dst[leftIndex + 2u] = borderColor;
      dst[leftIndex + 3u] = 255u;
    }
    if (rightIndex + 3u < context.framebuffer.size) {
      dst[rightIndex + 0u] = borderColor;
      dst[rightIndex + 1u] = borderColor;
      dst[rightIndex + 2u] = borderColor;
      dst[rightIndex + 3u] = 255u;
    }
  }
}
