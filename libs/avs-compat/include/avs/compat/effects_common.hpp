#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include "avs/effects/legacy_effect.h"

namespace avs::compat {

struct EffectConfigView {
  const std::uint8_t* data = nullptr;
  std::size_t size = 0;
};

std::string canonicalizeToken(std::string_view token,
                              const std::unordered_map<std::string, std::string>& aliases);

std::unique_ptr<effects::LegacyEffect> createLegacyEffect(
    std::string_view token,
    const std::unordered_map<std::string, std::string>& aliases,
    EffectConfigView config);

}  // namespace avs::compat
