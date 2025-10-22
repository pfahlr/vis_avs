#include "effects/trans/effect_mosaic.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace avs::effects::trans {

namespace {
constexpr int kOne = 1 << 16;
constexpr int kQualityMax = 100;
constexpr int kQualityMin = 1;

int clampQuality(int value) { return std::clamp(value, kQualityMin, kQualityMax); }

std::uint8_t saturatingAdd(std::uint8_t a, std::uint8_t b) {
  const int sum = static_cast<int>(a) + static_cast<int>(b);
  return static_cast<std::uint8_t>(sum > 255 ? 255 : sum);
}

}  // namespace

void Mosaic::ensureScratch(std::size_t bytes) {
  if (scratch_.size() < bytes) {
    scratch_.resize(bytes);
  }
}

std::uint32_t Mosaic::blendAdditive(std::uint32_t dst, std::uint32_t src) {
  const std::uint32_t r =
      saturatingAdd(static_cast<std::uint8_t>(dst & 0xFFu), static_cast<std::uint8_t>(src & 0xFFu));
  const std::uint32_t g = saturatingAdd(static_cast<std::uint8_t>((dst >> 8) & 0xFFu),
                                        static_cast<std::uint8_t>((src >> 8) & 0xFFu));
  const std::uint32_t b = saturatingAdd(static_cast<std::uint8_t>((dst >> 16) & 0xFFu),
                                        static_cast<std::uint8_t>((src >> 16) & 0xFFu));
  const std::uint32_t a = saturatingAdd(static_cast<std::uint8_t>((dst >> 24) & 0xFFu),
                                        static_cast<std::uint8_t>((src >> 24) & 0xFFu));
  return r | (g << 8) | (b << 16) | (a << 24);
}

std::uint32_t Mosaic::blendAverage(std::uint32_t dst, std::uint32_t src) {
  const std::uint16_t r = static_cast<std::uint16_t>(dst & 0xFFu) + static_cast<std::uint16_t>(src & 0xFFu);
  const std::uint16_t g = static_cast<std::uint16_t>((dst >> 8) & 0xFFu) +
                          static_cast<std::uint16_t>((src >> 8) & 0xFFu);
  const std::uint16_t b = static_cast<std::uint16_t>((dst >> 16) & 0xFFu) +
                          static_cast<std::uint16_t>((src >> 16) & 0xFFu);
  const std::uint16_t a = static_cast<std::uint16_t>((dst >> 24) & 0xFFu) +
                          static_cast<std::uint16_t>((src >> 24) & 0xFFu);
  return static_cast<std::uint32_t>(r >> 1) | (static_cast<std::uint32_t>(g >> 1) << 8) |
         (static_cast<std::uint32_t>(b >> 1) << 16) | (static_cast<std::uint32_t>(a >> 1) << 24);
}

void Mosaic::setParams(const avs::core::ParamBlock& params) {
  enabled_ = params.getBool("enabled", enabled_);
  quality_ = clampQuality(params.getInt("quality", quality_));
  if (params.contains("quality_onbeat")) {
    qualityOnBeat_ = clampQuality(params.getInt("quality_onbeat", qualityOnBeat_));
  } else if (params.contains("quality2")) {
    qualityOnBeat_ = clampQuality(params.getInt("quality2", qualityOnBeat_));
  } else {
    qualityOnBeat_ = clampQuality(params.getInt("quality_on_beat", qualityOnBeat_));
  }
  durationFrames_ = std::max(0, params.getInt("beat_duration", durationFrames_));
  if (params.contains("durFrames")) {
    durationFrames_ = std::max(0, params.getInt("durFrames", durationFrames_));
  }
  blendAdditive_ = params.getBool("blend", blendAdditive_);
  blendAverage_ = params.getBool("blendavg", blendAverage_);
  if (!params.contains("blendavg")) {
    blendAverage_ = params.getBool("blend_avg", blendAverage_);
  }
  triggerOnBeat_ = params.getBool("onbeat", triggerOnBeat_);
  if (!params.contains("onbeat")) {
    triggerOnBeat_ = params.getBool("on_beat", triggerOnBeat_);
  }

  if (remainingBeatFrames_ == 0) {
    currentQuality_ = quality_;
  }
}

bool Mosaic::render(avs::core::RenderContext& context) {
  if (!enabled_ || !hasFramebuffer(context)) {
    return true;
  }

  const int width = context.width;
  const int height = context.height;
  if (width <= 0 || height <= 0) {
    return true;
  }

  if (triggerOnBeat_ && context.audioBeat) {
    currentQuality_ = qualityOnBeat_;
    remainingBeatFrames_ = durationFrames_;
  } else if (remainingBeatFrames_ == 0) {
    currentQuality_ = quality_;
  }

  currentQuality_ = clampQuality(currentQuality_);
  const int effectiveQuality = currentQuality_;
  if (effectiveQuality < kQualityMin) {
    return true;
  }

  if (effectiveQuality < kQualityMax) {
    const std::size_t pixelCount =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    const std::size_t bytes = pixelCount * 4u;
    ensureScratch(bytes);
    std::uint8_t* framebuffer = context.framebuffer.data;
    std::memcpy(scratch_.data(), framebuffer, bytes);

    const auto* source = reinterpret_cast<const std::uint32_t*>(scratch_.data());
    auto* dst = reinterpret_cast<std::uint32_t*>(framebuffer);

    const int sXInc = (width * 65536) / effectiveQuality;
    const int sYInc = (height * 65536) / effectiveQuality;
    int ypos = sYInc >> 17;
    int dypos = 0;

    for (int y = 0; y < height; ++y) {
      if (ypos >= height) {
        break;
      }
      const int sampleRow = ypos * width;
      int dpos = 0;
      int xpos = sXInc >> 17;
      if (xpos >= width) {
        xpos = width - 1;
      }
      std::uint32_t srcPixel = source[sampleRow + xpos];
      for (int x = 0; x < width; ++x) {
        const std::size_t index = static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                                  static_cast<std::size_t>(x);
        const std::uint32_t basePixel = source[index];
        std::uint32_t result;
        if (blendAdditive_) {
          result = blendAdditive(basePixel, srcPixel);
        } else if (blendAverage_) {
          result = blendAverage(basePixel, srcPixel);
        } else {
          result = srcPixel;
        }
        dst[index] = result;

        dpos += kOne;
        if (dpos >= sXInc) {
          xpos += dpos >> 16;
          if (xpos >= width) {
            break;
          }
          srcPixel = source[sampleRow + xpos];
          dpos -= sXInc;
        }
      }

      dypos += kOne;
      if (dypos >= sYInc) {
        ypos += dypos >> 16;
        if (ypos >= height) {
          break;
        }
        dypos -= sYInc;
      }
    }
  }

  if (remainingBeatFrames_ > 0) {
    --remainingBeatFrames_;
    if (remainingBeatFrames_ > 0 && durationFrames_ > 0) {
      const int delta = std::abs(quality_ - qualityOnBeat_) / durationFrames_;
      if (delta > 0) {
        if (qualityOnBeat_ > quality_) {
          currentQuality_ = clampQuality(currentQuality_ - delta);
        } else if (qualityOnBeat_ < quality_) {
          currentQuality_ = clampQuality(currentQuality_ + delta);
        }
      }
    }
  }

  return true;
}

}  // namespace avs::effects::trans
