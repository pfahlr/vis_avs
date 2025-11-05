#include "avs/compat/effects_render.hpp"

#include <string>
#include <string_view>
#include <unordered_map>

namespace avs::compat::render {

namespace {
const std::unordered_map<std::string, std::string>& aliasMap() {
  static const std::unordered_map<std::string, std::string> kAliases = {
      {"render/oscilloscope star", "Render / Oscilloscope Star"},
      {"render / oscilloscope star", "Render / Oscilloscope Star"},
  };
  return kAliases;
}

bool isRenderToken(std::string_view token) {
  return token.rfind("Render /", 0) == 0;
}

}  // namespace

std::unique_ptr<effects::LegacyEffect> instantiate(std::string_view token,
                                                    EffectConfigView config) {
  const std::string canonical = canonicalizeToken(token, aliasMap());
  if (!isRenderToken(canonical)) {
    return nullptr;
  }
  return createLegacyEffect(canonical, aliasMap(), config);
}

std::vector<std::uint8_t> serialize(const effects::LegacyEffect& effect) {
  return effect.saveConfig();
}

}  // namespace avs::compat::render
