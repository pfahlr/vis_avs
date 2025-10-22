#include <avs/effects/registry.h>

#include <algorithm>
#include <cctype>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace avs::effects {

void Registry::add(Descriptor d) {
  if (d.id.empty() || !d.factory) {
    return;
  }
  const std::string idKey = d.id;
  byId_[idKey] = std::move(d);
  const Descriptor& ref = byId_.at(idKey);
  for (const auto& legacy : ref.legacy_tokens) {
    const std::string norm = normalize_legacy_token(legacy);
    if (!norm.empty()) {
      legacy2id_[norm] = ref.id;
    }
  }
}

const Descriptor* Registry::by_id(std::string_view id) const {
  auto it = byId_.find(std::string(id));
  if (it == byId_.end()) {
    return nullptr;
  }
  return &it->second;
}

const Descriptor* Registry::by_legacy(std::string_view legacy_token_norm) const {
  auto it = legacy2id_.find(std::string(legacy_token_norm));
  if (it == legacy2id_.end()) {
    return nullptr;
  }
  return by_id(it->second);
}

std::unique_ptr<IEffect> Registry::make(std::string_view token_or_id, const ParamList& params,
                                        const BuildCtx& ctx, bool* matched_legacy) const {
  if (matched_legacy) {
    *matched_legacy = false;
  }
  if (const Descriptor* byId = by_id(token_or_id)) {
    return byId->factory(params, ctx);
  }
  const std::string norm = normalize_legacy_token(token_or_id);
  if (const Descriptor* legacyDesc = by_legacy(norm)) {
    if (matched_legacy) {
      *matched_legacy = true;
    }
    return legacyDesc->factory(params, ctx);
  }
  return nullptr;
}

std::string Registry::normalize_legacy_token(std::string_view s) {
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

}  // namespace avs::effects
