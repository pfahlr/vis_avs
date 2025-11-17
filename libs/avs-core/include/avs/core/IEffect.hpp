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
   * @brief Update effect parameters prior to rendering.
   *
   * @param params Parameter block supplied by the preset or caller.
   */
  virtual void setParams(const ParamBlock& params) = 0;
};

}  // namespace avs::core
