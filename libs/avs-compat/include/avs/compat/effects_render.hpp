#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "avs/compat/effects_common.hpp"

namespace avs::compat::render {

std::unique_ptr<effects::LegacyEffect> instantiate(std::string_view token,
                                                    EffectConfigView config = {});

std::vector<std::uint8_t> serialize(const effects::LegacyEffect& effect);

}  // namespace avs::compat::render
