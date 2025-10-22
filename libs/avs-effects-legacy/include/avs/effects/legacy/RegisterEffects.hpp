#pragma once

#include <avs/core/EffectRegistry.hpp>

namespace avs::effects {

/**
 * @brief Register the built-in core effects with the provided registry.
 */
void registerCoreEffects(avs::core::EffectRegistry& registry);

}  // namespace avs::effects
