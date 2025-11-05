#include <avs/effects/effect_registry.hpp>

#include <cctype>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace avs::effects::legacy {
namespace {
std::unordered_map<std::string, EffectFactory>& registry() {
  static std::unordered_map<std::string, EffectFactory> factories;
  return factories;
}
}  // namespace

std::string normalizeLegacyToken(std::string_view s) {
  std::string out;
  out.reserve(s.size());
  bool lastWasSlash = false;
  bool pendingUnderscore = false;
  for (size_t i = 0; i < s.size(); ++i) {
    unsigned char ch = static_cast<unsigned char>(s[i]);
    if (std::isspace(ch) || ch == '-' || ch == '_') {
      if (lastWasSlash) {
        continue;
      }
      pendingUnderscore = true;
      continue;
    }
    char lower = static_cast<char>(std::tolower(ch));
    if (lower == '/' || lower == '\\') {
      if (!out.empty() && out.back() == '_') {
        out.pop_back();
      }
      out.push_back('/');
      lastWasSlash = true;
      pendingUnderscore = false;
      continue;
    }
    if (pendingUnderscore && !out.empty() && out.back() != '/' && out.back() != '_') {
      out.push_back('_');
    }
    pendingUnderscore = false;
    out.push_back(lower);
    lastWasSlash = false;
  }
  if (!out.empty() && out.back() == '_') {
    out.pop_back();
  }
  return out;
}

void RegisterEffectFactory(std::string token, EffectFactory factory) {
  const std::string key = normalizeLegacyToken(token);
  registry()[key] = std::move(factory);
}

const std::unordered_map<std::string, EffectFactory>& GetEffectRegistry() { return registry(); }

}  // namespace avs::effects::legacy
