#pragma once

#include <avs/preset/ir.h>

#include <string>
#include <string_view>

namespace avs::preset {

IRPreset parse_legacy_preset(std::string_view text);

}  // namespace avs::preset

namespace avs::runtime::parser {

std::string normalizeEffectToken(std::string_view token);

}  // namespace avs::runtime::parser
