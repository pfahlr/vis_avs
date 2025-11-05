#include "avs/compat/effects_common.hpp"

#include <algorithm>
#include <cctype>
#include <string>

#include "avs/effects/effect_registry.hpp"

namespace avs::compat {

namespace {
std::string trimToken(std::string_view token) {
  auto begin = token.find_first_not_of(" \t\r\n");
  auto end = token.find_last_not_of(" \t\r\n");
  if (begin == std::string_view::npos) {
    return std::string{};
  }
  return std::string(token.substr(begin, end - begin + 1));
}

std::string normalizeCase(std::string_view token) {
  std::string result(token);
  std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return result;
}
}  // namespace

std::string canonicalizeToken(std::string_view token,
                              const std::unordered_map<std::string, std::string>& aliases) {
  const std::string trimmed = trimToken(token);
  if (trimmed.empty()) {
    return {};
  }
  auto it = aliases.find(normalizeCase(trimmed));
  if (it != aliases.end()) {
    return it->second;
  }
  return trimmed;
}

std::unique_ptr<effects::LegacyEffect> createLegacyEffect(
    std::string_view token,
    const std::unordered_map<std::string, std::string>& aliases,
    EffectConfigView config) {
  const std::string canonical = canonicalizeToken(token, aliases);
  auto effect = effects::EffectRegistry::instance().create(canonical);
  if (!effect) {
    return nullptr;
  }
  if (config.data != nullptr && config.size > 0) {
    effect->loadConfig(config.data, config.size);
  }
  return effect;
}

}  // namespace avs::compat
