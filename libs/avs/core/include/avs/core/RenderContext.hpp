#pragma once

#include <cstddef>
#include <cstdint>

#include "avs/core/DeterministicRng.hpp"

namespace avs::audio {
struct Analysis;
}

namespace avs::core {

/**
 * @brief View into a mutable pixel buffer.
 */
struct PixelBufferView {
  std::uint8_t* data = nullptr;
  std::size_t size = 0;
};

/**
 * @brief View into an immutable audio analysis buffer.
 */
struct AudioBufferView {
  const float* data = nullptr;
  std::size_t size = 0;
};

/**
 * @brief Per-frame rendering state passed to every effect.
 */
struct RenderContext {
  std::uint64_t frameIndex = 0;
  double deltaSeconds = 0.0;
  int width = 0;
  int height = 0;
  PixelBufferView framebuffer;
  AudioBufferView audioSpectrum;
  bool audioBeat = false;
  const avs::audio::Analysis* audioAnalysis = nullptr;
  DeterministicRng rng;
};

}  // namespace avs::core
