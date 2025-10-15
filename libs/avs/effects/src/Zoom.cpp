#include "avs/effects/Zoom.hpp"

namespace avs::effects {

bool Zoom::render(avs::core::RenderContext& context) {
  (void)context;
  return true;
}

void Zoom::setParams(const avs::core::ParamBlock& params) {
  factor_ = params.getFloat("factor", factor_);
}

}  // namespace avs::effects
