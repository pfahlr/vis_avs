#pragma once

#include <avs/core/ParamBlock.hpp>
#include <avs/core/RenderContext.hpp>

namespace avs::core {

/**
 * @brief Interface implemented by all renderable effects in the pipeline.
 */
class IEffect {
 public:
  virtual ~IEffect() = default;

  /**
   * @brief Render the effect for the provided frame.
   *
   * @param context Mutable rendering context for the current frame.
   * @return true when rendering succeeded, false when the effect should halt
   * further processing.
   */
  virtual bool render(RenderContext& context) = 0;

  /**
   * @brief Multi-threaded render method (optional).
   *
   * Effects can optionally override this method to provide multi-threaded
   * rendering. The default implementation falls back to single-threaded render().
   *
   * @param context Mutable rendering context for the current frame.
   * @param threadId Current thread ID (0-indexed).
   * @param maxThreads Total number of threads available.
   * @return true when rendering succeeded, false when the effect should halt
   * further processing.
   */
  virtual bool smp_render(RenderContext& context, int threadId, int /* maxThreads */) {
    // Default implementation: only thread 0 renders (single-threaded fallback)
    if (threadId == 0) {
      return render(context);
    }
    return true;
  }

  /**
   * @brief Check if this effect supports multi-threaded rendering.
   *
   * @return true if the effect has overridden smp_render, false otherwise.
   */
  virtual bool supportsMultiThreaded() const { return false; }

  /**
   * @brief Update effect parameters prior to rendering.
   *
   * @param params Parameter block supplied by the preset or caller.
   */
  virtual void setParams(const ParamBlock& params) = 0;
};

}  // namespace avs::core
