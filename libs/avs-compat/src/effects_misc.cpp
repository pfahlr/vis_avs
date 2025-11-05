#include "avs/compat/effects_misc.hpp"

#include <string>
#include <string_view>
#include <unordered_map>

namespace avs::compat::misc {

namespace {
const std::unordered_map<std::string, std::string>& aliasMap() {
  static const std::unordered_map<std::string, std::string> kAliases = {
      {"misc/unknown render object", "Unknown Render Object"},
      {"unknown render object", "Unknown Render Object"},
  };
  return kAliases;
}

bool isMiscToken(std::string_view token) {
  return token == "Unknown Render Object";
}

}  // namespace

std::unique_ptr<effects::LegacyEffect> instantiate(std::string_view token,
                                                    EffectConfigView config) {
  const std::string canonical = canonicalizeToken(token, aliasMap());
  if (!isMiscToken(canonical)) {
    return nullptr;
  }
  return createLegacyEffect(canonical, aliasMap(), config);
}

std::vector<std::uint8_t> serialize(const effects::LegacyEffect& effect) {
  return effect.saveConfig();
}

}  // namespace avs::compat::misc
