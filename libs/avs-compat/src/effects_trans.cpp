#include "avs/compat/effects_trans.hpp"

#include <string>
#include <string_view>
#include <unordered_map>

namespace avs::compat::trans {

namespace {
const std::unordered_map<std::string, std::string>& aliasMap() {
  static const std::unordered_map<std::string, std::string> kAliases = {
      {"trans/fadeout", "Trans / Fadeout"},
      {"trans / fadeout", "Trans / Fadeout"},
      {"core/fadeout", "Trans / Fadeout"},
      {"prime/transition", "Trans / Transition"},
      {"trans/transition", "Trans / Transition"},
  };
  return kAliases;
}

bool isTransToken(std::string_view token) {
  return token.rfind("Trans /", 0) == 0;
}

}  // namespace

std::unique_ptr<effects::LegacyEffect> instantiate(std::string_view token,
                                                    EffectConfigView config) {
  const std::string canonical = canonicalizeToken(token, aliasMap());
  if (!isTransToken(canonical)) {
    return nullptr;
  }
  return createLegacyEffect(canonical, aliasMap(), config);
}

std::vector<std::uint8_t> serialize(const effects::LegacyEffect& effect) {
  return effect.saveConfig();
}

}  // namespace avs::compat::trans
