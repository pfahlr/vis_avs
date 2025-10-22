#include "effects/filters/effect_grain.h"

#include <algorithm>
#include <random>

#include "effects/filters/filter_common.h"

namespace avs::effects::filters {

namespace {
constexpr int kMaxAmount = 255;
}

void Grain::setParams(const avs::core::ParamBlock& params) {
  amount_ = std::clamp(params.getInt("amount", amount_), 0, kMaxAmount);
  monochrome_ = params.getBool("monochrome", monochrome_);
  staticGrain_ = params.getBool("static", staticGrain_);
  seedOffset_ = params.getInt("seed", seedOffset_);
  dirty_ = true;
}

void Grain::regenerateStaticPattern(int width, int height, std::uint64_t seedBase) {
  patternWidth_ = width;
  patternHeight_ = height;
  patternSeed_ = seedBase;
  staticPattern_.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 3u, 0);
  std::mt19937 rng(static_cast<std::mt19937::result_type>(seedBase));
  if (amount_ <= 0) {
    std::fill(staticPattern_.begin(), staticPattern_.end(), 0);
    dirty_ = false;
    return;
  }
  std::uniform_int_distribution<int> dist(-amount_, amount_);
  const std::size_t pixels = staticPattern_.size() / 3u;
  for (std::size_t i = 0; i < pixels; ++i) {
    if (monochrome_) {
      const int value = dist(rng);
      staticPattern_[i * 3u + 0] = value;
      staticPattern_[i * 3u + 1] = value;
      staticPattern_[i * 3u + 2] = value;
    } else {
      staticPattern_[i * 3u + 0] = dist(rng);
      staticPattern_[i * 3u + 1] = dist(rng);
      staticPattern_[i * 3u + 2] = dist(rng);
    }
  }
  dirty_ = false;
}

bool Grain::render(avs::core::RenderContext& context) {
  if (!hasFramebuffer(context) || amount_ <= 0) {
    return true;
  }

  const int width = context.width;
  const int height = context.height;
  std::uint8_t* pixels = context.framebuffer.data;
  const std::size_t totalPixels = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);

  if (staticGrain_) {
    const std::uint64_t seedBase = context.rng.seed() ^ (static_cast<std::uint64_t>(seedOffset_) + 0x9E3779B97F4A7C15ull);
    if (dirty_ || width != patternWidth_ || height != patternHeight_ || patternSeed_ != seedBase) {
      regenerateStaticPattern(width, height, seedBase);
    }
    const int* pattern = staticPattern_.data();
    for (std::size_t i = 0; i < totalPixels; ++i) {
      std::uint8_t* px = pixels + i * 4u;
      px[0] = saturatingAdd(px[0], pattern[i * 3u + 0]);
      px[1] = saturatingAdd(px[1], pattern[i * 3u + 1]);
      px[2] = saturatingAdd(px[2], pattern[i * 3u + 2]);
    }
    return true;
  }

  std::uint32_t frameSeed = context.rng.nextUint32();
  frameSeed ^= static_cast<std::uint32_t>(seedOffset_);
  std::mt19937 rng(frameSeed ? frameSeed : 0xA5A5A5A5u);
  std::uniform_int_distribution<int> dist(-amount_, amount_);
  for (std::size_t i = 0; i < totalPixels; ++i) {
    std::uint8_t* px = pixels + i * 4u;
    if (monochrome_) {
      const int value = dist(rng);
      px[0] = saturatingAdd(px[0], value);
      px[1] = saturatingAdd(px[1], value);
      px[2] = saturatingAdd(px[2], value);
    } else {
      px[0] = saturatingAdd(px[0], dist(rng));
      px[1] = saturatingAdd(px[1], dist(rng));
      px[2] = saturatingAdd(px[2], dist(rng));
    }
  }
  return true;
}

}  // namespace avs::effects::filters

