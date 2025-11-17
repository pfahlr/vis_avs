#include "avs/effects/misc/effect_unknown_render_object.h"

#include <utility>

namespace avs::effects::misc {

// NOTE: This effect uses the LegacyEffect interface which is incompatible with
// the development-branch EffectRegistry that expects IEffect-derived classes.
// To register this effect, it needs to be adapted to implement IEffect or
// wrapped in an adapter factory function (see effect_registry.cpp for examples).

void EffectUnknownRenderObject::render(LegacyRenderContext& context) {
  (void)context;
}

void EffectUnknownRenderObject::loadConfig(const std::uint8_t* data, std::size_t size) {
  if (data == nullptr || size == 0) {
    rawConfig_.clear();
    return;
  }
  rawConfig_.assign(data, data + size);
}

std::vector<std::uint8_t> EffectUnknownRenderObject::saveConfig() const {
  return rawConfig_;
}

void EffectUnknownRenderObject::setOriginalToken(std::string token) {
  originalToken_ = std::move(token);
}

}  // namespace avs::effects::misc

