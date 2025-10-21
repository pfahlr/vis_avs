#include "effects/render/effect_avi.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "stb_image.h"

namespace {
constexpr std::size_t kChannels = 4u;

[[nodiscard]] std::string changeExtension(const std::string& path, const std::string& extension) {
  if (path.empty()) {
    return {};
  }
  const auto pos = path.find_last_of('.');
  if (pos == std::string::npos) {
    return path + extension;
  }
  return path.substr(0, pos) + extension;
}

[[nodiscard]] bool isFramebufferLargeEnough(const avs::core::RenderContext& context) {
  if (!context.framebuffer.data) {
    return false;
  }
  const std::size_t expected = static_cast<std::size_t>(context.width) *
                               static_cast<std::size_t>(context.height) * kChannels;
  return context.framebuffer.size >= expected;
}
}  // namespace

Effect_RenderAvi::Effect_RenderAvi() = default;

Effect_RenderAvi::~Effect_RenderAvi() = default;

bool Effect_RenderAvi::render(avs::core::RenderContext& context) {
  if (!enabled_) {
    return true;
  }
  if (context.width <= 0 || context.height <= 0 || !isFramebufferLargeEnough(context)) {
    return true;
  }

  ensureThumbnailLoaded();

  ImageBuffer thumbnailCopy;
  {
    std::lock_guard<std::mutex> guard(loadMutex_);
    thumbnailCopy = thumbnail_;
  }

  if (thumbnailCopy.valid()) {
    fillBackground(context);
    drawThumbnail(context, thumbnailCopy);
  } else {
    drawPlaceholder(context);
  }

  return true;
}

void Effect_RenderAvi::setParams(const avs::core::ParamBlock& params) {
  const bool newEnabled = params.getBool("enabled", enabled_);
  const std::string newSource = params.getString("source", sourcePath_);
  const std::string newFallback = params.getString("fallback_image", fallbackImagePath_);
  const std::string alternateFallback = params.getString("thumbnail", newFallback);

  std::lock_guard<std::mutex> guard(loadMutex_);
  enabled_ = newEnabled;

  bool invalidateThumbnail = false;
  if (newSource != sourcePath_) {
    sourcePath_ = newSource;
    invalidateThumbnail = true;
  }

  std::string resolvedFallback = !alternateFallback.empty() ? alternateFallback : newFallback;
  if (resolvedFallback != fallbackImagePath_) {
    fallbackImagePath_ = std::move(resolvedFallback);
    invalidateThumbnail = true;
  }

  if (invalidateThumbnail) {
    attemptedLoad_ = false;
    thumbnail_ = {};
  }
}

void Effect_RenderAvi::ensureThumbnailLoaded() {
  std::lock_guard<std::mutex> guard(loadMutex_);
  if (attemptedLoad_) {
    return;
  }
  attemptedLoad_ = true;

  std::vector<std::string> candidates;
  if (!fallbackImagePath_.empty()) {
    candidates.push_back(fallbackImagePath_);
  }
  if (!sourcePath_.empty()) {
    candidates.push_back(changeExtension(sourcePath_, ".png"));
    candidates.push_back(changeExtension(sourcePath_, ".jpg"));
    candidates.push_back(changeExtension(sourcePath_, ".jpeg"));
  }

  for (const auto& candidate : candidates) {
    if (candidate.empty()) {
      continue;
    }
    int width = 0;
    int height = 0;
    int comp = 0;
    unsigned char* data = stbi_load(candidate.c_str(), &width, &height, &comp, static_cast<int>(kChannels));
    if (!data) {
      continue;
    }

    try {
      thumbnail_.width = width;
      thumbnail_.height = height;
      const std::size_t expectedSize = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * kChannels;
      thumbnail_.pixels.assign(data, data + expectedSize);
      std::clog << "Render / AVI: loaded fallback thumbnail from " << candidate << '\n';
    } catch (...) {
      stbi_image_free(data);
      thumbnail_ = {};
      throw;
    }

    stbi_image_free(data);
    if (thumbnail_.valid()) {
      return;
    }
  }

  if (!candidates.empty()) {
    std::clog << "Render / AVI: no fallback image available for " << sourcePath_ << '\n';
  }
}

void Effect_RenderAvi::fillBackground(avs::core::RenderContext& context) const {
  auto* dst = context.framebuffer.data;
  const int width = context.width;
  const int height = context.height;
  for (int y = 0; y < height; ++y) {
    const float fy = height > 1 ? static_cast<float>(y) / static_cast<float>(height - 1) : 0.0f;
    for (int x = 0; x < width; ++x) {
      const float fx = width > 1 ? static_cast<float>(x) / static_cast<float>(width - 1) : 0.0f;
      const std::size_t idx =
          (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)) * kChannels;

      const float base = 40.0f + 60.0f * fx;
      const float accent = 50.0f + 150.0f * (1.0f - fy);

      dst[idx + 0u] = static_cast<std::uint8_t>(std::clamp(base + accent * 0.2f, 0.0f, 255.0f));
      dst[idx + 1u] = static_cast<std::uint8_t>(std::clamp(base + accent * 0.1f, 0.0f, 255.0f));
      dst[idx + 2u] = static_cast<std::uint8_t>(std::clamp(base + accent * 0.3f, 0.0f, 255.0f));
      dst[idx + 3u] = 255u;
    }
  }
}

void Effect_RenderAvi::drawPlaceholder(avs::core::RenderContext& context) const {
  fillBackground(context);
  auto* dst = context.framebuffer.data;
  const int width = context.width;
  const int height = context.height;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const std::size_t idx =
          (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)) * kChannels;
      const bool stripe = ((x / 12) + (y / 12)) % 2 == 0;
      if (stripe) {
        dst[idx + 0u] = static_cast<std::uint8_t>(std::clamp<int>(dst[idx + 0u] + 18, 0, 255));
        dst[idx + 1u] = static_cast<std::uint8_t>(std::clamp<int>(dst[idx + 1u] + 18, 0, 255));
        dst[idx + 2u] = static_cast<std::uint8_t>(std::clamp<int>(dst[idx + 2u] + 18, 0, 255));
      }
    }
  }

  const int iconWidth = std::max(16, width / 3);
  const int iconHeight = std::max(16, height / 3);
  const int iconX0 = (width - iconWidth) / 2;
  const int iconY0 = (height - iconHeight) / 2;
  const int iconX1 = iconX0 + iconWidth;
  const int iconY1 = iconY0 + iconHeight;

  for (int y = iconY0; y < iconY1; ++y) {
    if (y < 0 || y >= height) {
      continue;
    }
    for (int x = iconX0; x < iconX1; ++x) {
      if (x < 0 || x >= width) {
        continue;
      }
      const std::size_t idx =
          (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)) * kChannels;
      const bool border = (y == iconY0 || y == iconY1 - 1 || x == iconX0 || x == iconX1 - 1);
      if (border) {
        dst[idx + 0u] = static_cast<std::uint8_t>(std::clamp<int>(dst[idx + 0u] + 40, 0, 255));
        dst[idx + 1u] = static_cast<std::uint8_t>(std::clamp<int>(dst[idx + 1u] + 40, 0, 255));
        dst[idx + 2u] = static_cast<std::uint8_t>(std::clamp<int>(dst[idx + 2u] + 40, 0, 255));
        continue;
      }
      dst[idx + 0u] = static_cast<std::uint8_t>(std::clamp<int>(dst[idx + 0u] + 65, 0, 255));
      dst[idx + 1u] = static_cast<std::uint8_t>(std::clamp<int>(dst[idx + 1u] + 35, 0, 255));
      dst[idx + 2u] = static_cast<std::uint8_t>(std::clamp<int>(dst[idx + 2u] + 20, 0, 255));
    }
  }
}

void Effect_RenderAvi::drawThumbnail(avs::core::RenderContext& context, const ImageBuffer& thumbnail) const {
  auto* dst = context.framebuffer.data;
  if (!thumbnail.valid()) {
    return;
  }

  const int width = context.width;
  const int height = context.height;
  const float scaleX = static_cast<float>(width) / static_cast<float>(thumbnail.width);
  const float scaleY = static_cast<float>(height) / static_cast<float>(thumbnail.height);
  const float scale = std::min(scaleX, scaleY);
  const int targetWidth = std::max(1, static_cast<int>(std::round(static_cast<float>(thumbnail.width) * scale)));
  const int targetHeight = std::max(1, static_cast<int>(std::round(static_cast<float>(thumbnail.height) * scale)));
  const int offsetX = (width - targetWidth) / 2;
  const int offsetY = (height - targetHeight) / 2;

  for (int y = 0; y < targetHeight; ++y) {
    const int destY = offsetY + y;
    if (destY < 0 || destY >= height) {
      continue;
    }
    const float v = targetHeight > 1 ? static_cast<float>(y) / static_cast<float>(targetHeight - 1) : 0.0f;
    const int srcY = std::clamp(static_cast<int>(std::round(v * static_cast<float>(thumbnail.height - 1))), 0,
                                thumbnail.height - 1);

    for (int x = 0; x < targetWidth; ++x) {
      const int destX = offsetX + x;
      if (destX < 0 || destX >= width) {
        continue;
      }
      const float u = targetWidth > 1 ? static_cast<float>(x) / static_cast<float>(targetWidth - 1) : 0.0f;
      const int srcX = std::clamp(static_cast<int>(std::round(u * static_cast<float>(thumbnail.width - 1))), 0,
                                  thumbnail.width - 1);

      const std::size_t dstIdx =
          (static_cast<std::size_t>(destY) * static_cast<std::size_t>(width) + static_cast<std::size_t>(destX)) *
          kChannels;
      const std::size_t srcIdx =
          (static_cast<std::size_t>(srcY) * static_cast<std::size_t>(thumbnail.width) +
           static_cast<std::size_t>(srcX)) *
          kChannels;

      dst[dstIdx + 0u] = thumbnail.pixels[srcIdx + 0u];
      dst[dstIdx + 1u] = thumbnail.pixels[srcIdx + 1u];
      dst[dstIdx + 2u] = thumbnail.pixels[srcIdx + 2u];
      dst[dstIdx + 3u] = 255u;
    }
  }
}
