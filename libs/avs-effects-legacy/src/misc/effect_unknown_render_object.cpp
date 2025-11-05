#include "avs/effects/misc/effect_unknown_render_object.h"

#include <utility>

#include "avs/effects/effect_registry.hpp"

namespace avs::effects::misc {

AVS_EFFECT_TOKEN("Unknown Render Object");
REGISTER_AVS_EFFECT(EffectUnknownRenderObject, "Unknown Render Object");

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

