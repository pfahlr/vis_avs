#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <avs/core/DeterministicRng.hpp>
#include <avs/core/ParamBlock.hpp>
#include <avs/core/RenderContext.hpp>
#include <avs/effects/legacy/trans/effect_scatter.h>

namespace {

constexpr int kFalloffRadius = 4;
constexpr int kOffsetTableSize = 512;
constexpr int kOffsetMask = kOffsetTableSize - 1;
constexpr int kKernelSize = 8;

avs::core::RenderContext makeContext(std::vector<std::uint8_t>& buffer, int width, int height) {
  avs::core::RenderContext context{};
  context.width = width;
  context.height = height;
  context.framebuffer = {buffer.data(), buffer.size()};
  context.audioBeat = false;
  return context;
}

std::vector<std::uint8_t> makeSequentialPattern(int width, int height) {
  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width) *
                                   static_cast<std::size_t>(height) * 4u);
  auto* words = reinterpret_cast<std::uint32_t*>(pixels.data());
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const std::size_t index = static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                                static_cast<std::size_t>(x);
      words[index] = static_cast<std::uint32_t>((index + 1u) * 0x01020408u);
    }
  }
  return pixels;
}

std::uint32_t lerpColor(std::uint32_t original, std::uint32_t scattered, int weight, int scale) {
  const int inv = scale - weight;
  const int bias = scale / 2;
  const auto blendChannel = [&](int shift) {
    const int base = static_cast<int>((original >> shift) & 0xFFu);
    const int sample = static_cast<int>((scattered >> shift) & 0xFFu);
    const int value = base * inv + sample * weight + bias;
    return static_cast<std::uint32_t>(std::clamp(value / scale, 0, 255)) << shift;
  };
  return blendChannel(0) | blendChannel(8) | blendChannel(16) | blendChannel(24);
}

std::vector<int> buildOffsets(int width) {
  std::vector<int> offsets(kOffsetTableSize);
  for (int i = 0; i < kOffsetTableSize; ++i) {
    int offsetX = (i % kKernelSize) - (kKernelSize / 2);
    int offsetY = ((i / kKernelSize) % kKernelSize) - (kKernelSize / 2);
    if (offsetX < 0) {
      ++offsetX;
    }
    if (offsetY < 0) {
      ++offsetY;
    }
    offsets[i] = offsetY * width + offsetX;
  }
  return offsets;
}

std::vector<std::uint8_t> runReferenceScatter(const std::vector<std::uint8_t>& base,
                                             int width,
                                             int height,
                                             std::uint64_t seed,
                                             std::uint64_t frameIndex) {
  std::vector<std::uint8_t> result = base;
  if (width <= 0 || height <= 0) {
    return result;
  }

  auto offsets = buildOffsets(width);
  avs::core::DeterministicRng rng(seed);
  rng.reseed(frameIndex);

  const auto* source = reinterpret_cast<const std::uint32_t*>(base.data());
  auto* dst = reinterpret_cast<std::uint32_t*>(result.data());
  const int totalPixels = width * height;

  for (int y = 0; y < height; ++y) {
    const int distanceY = std::min(y, height - 1 - y);
    for (int x = 0; x < width; ++x) {
      const int index = y * width + x;
      const std::uint32_t basePixel = source[static_cast<std::size_t>(index)];
      const int distanceX = std::min(x, width - 1 - x);
      const int distance = std::max(0, std::min(distanceX, distanceY));
      const int weight = std::min(distance, kFalloffRadius);
      if (weight <= 0) {
        dst[static_cast<std::size_t>(index)] = basePixel;
        continue;
      }

      const std::uint32_t randomValue = rng.nextUint32();
      const int tableIndex = static_cast<int>(randomValue & kOffsetMask);
      int sampleIndex = index + offsets[tableIndex];
      sampleIndex = std::clamp(sampleIndex, 0, totalPixels - 1);
      const std::uint32_t scatteredPixel = source[static_cast<std::size_t>(sampleIndex)];
      if (weight >= kFalloffRadius) {
        dst[static_cast<std::size_t>(index)] = scatteredPixel;
      } else {
        dst[static_cast<std::size_t>(index)] = lerpColor(basePixel, scatteredPixel, weight, kFalloffRadius);
      }
    }
  }

  return result;
}

}  // namespace

TEST(ScatterEffect, DisabledNoOp) {
  constexpr int kWidth = 12;
  constexpr int kHeight = 10;
  auto base = makeSequentialPattern(kWidth, kHeight);
  auto working = base;

  avs::effects::trans::Scatter effect;
  avs::core::ParamBlock params;
  params.setBool("enabled", false);
  effect.setParams(params);

  auto context = makeContext(working, kWidth, kHeight);
  context.frameIndex = 3;
  context.rng = avs::core::DeterministicRng(1234);
  context.rng.reseed(context.frameIndex);

  ASSERT_TRUE(effect.render(context));
  EXPECT_EQ(working, base);
}

TEST(ScatterEffect, MatchesReferenceImplementation) {
  constexpr int kWidth = 24;
  constexpr int kHeight = 18;
  auto base = makeSequentialPattern(kWidth, kHeight);
  auto working = base;

  avs::effects::trans::Scatter effect;
  avs::core::ParamBlock params;
  params.setBool("enabled", true);
  effect.setParams(params);

  auto context = makeContext(working, kWidth, kHeight);
  context.frameIndex = 42;
  const std::uint64_t seed = 0xABCDEF01ull;
  context.rng = avs::core::DeterministicRng(seed);
  context.rng.reseed(context.frameIndex);

  ASSERT_TRUE(effect.render(context));

  auto expected = runReferenceScatter(base, kWidth, kHeight, seed, context.frameIndex);
  EXPECT_EQ(working, expected);
}

TEST(ScatterEffect, PreservesEdgePixels) {
  constexpr int kWidth = 16;
  constexpr int kHeight = 12;
  auto base = makeSequentialPattern(kWidth, kHeight);
  auto working = base;

  avs::effects::trans::Scatter effect;
  avs::core::ParamBlock params;
  params.setBool("enabled", true);
  effect.setParams(params);

  auto context = makeContext(working, kWidth, kHeight);
  context.frameIndex = 5;
  context.rng = avs::core::DeterministicRng(987654321ull);
  context.rng.reseed(context.frameIndex);

  ASSERT_TRUE(effect.render(context));

  const auto* outPixels = reinterpret_cast<const std::uint32_t*>(working.data());
  const auto* basePixels = reinterpret_cast<const std::uint32_t*>(base.data());

  for (int y = 0; y < kHeight; ++y) {
    for (int x = 0; x < kWidth; ++x) {
      const int distanceX = std::min(x, kWidth - 1 - x);
      const int distanceY = std::min(y, kHeight - 1 - y);
      const int distance = std::max(0, std::min(distanceX, distanceY));
      if (distance == 0) {
        const std::size_t pixelIndex = static_cast<std::size_t>(y) * static_cast<std::size_t>(kWidth) +
                                       static_cast<std::size_t>(x);
        EXPECT_EQ(outPixels[pixelIndex], basePixels[pixelIndex])
            << "Edge pixel mismatch at (" << x << ", " << y << ")";
      }
    }
  }
}

