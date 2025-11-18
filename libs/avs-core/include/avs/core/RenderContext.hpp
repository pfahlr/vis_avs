#pragma once

#include <cstddef>
#include <cstdint>

#include <avs/core/DeterministicRng.hpp>

namespace avs::runtime {
struct GlobalState;
}

namespace avs::audio {
struct Analysis;
}

namespace avs::core {

class IFramebuffer;

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
 *
 * @note Modern effects should prefer using `framebufferBackend` over the legacy
 *       `framebuffer` view. The backend provides additional functionality like
 *       upload/download, resize, and backend-specific optimizations.
 */
struct RenderContext {
  std::uint64_t frameIndex = 0;
  double deltaSeconds = 0.0;
  int width = 0;
  int height = 0;

  /**
   * @brief Framebuffer backend (modern interface, optional).
   *
   * When set, effects should prefer this over the legacy framebuffer view.
   * Provides upload(), download(), resize(), clear(), and backend metadata.
   * May be nullptr for legacy code paths.
   */
  IFramebuffer* framebufferBackend = nullptr;

  /**
   * @brief Legacy pixel buffer view (deprecated, use framebufferBackend).
   *
   * Maintained for backward compatibility. New effects should use
   * framebufferBackend->data() instead of framebuffer.data directly.
   */
  PixelBufferView framebuffer;

  AudioBufferView audioSpectrum;
  bool audioBeat = false;
  const avs::audio::Analysis* audioAnalysis = nullptr;
  avs::runtime::GlobalState* globals = nullptr;
  DeterministicRng rng;
};

}  // namespace avs::core
