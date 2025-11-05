#include "avs/effects/effect_registry.hpp"

#include <utility>

namespace avs::effects {

EffectRegistry& EffectRegistry::instance() {
  static EffectRegistry registry;
  return registry;
}

bool EffectRegistry::registerEffect(const std::string& token, Factory factory) {
  std::lock_guard<std::mutex> lock(mutex_);
  return factories_.emplace(token, std::move(factory)).second;
}

std::unique_ptr<LegacyEffect> EffectRegistry::create(const std::string& token) const {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = factories_.find(token);
  if (it == factories_.end()) {
    return nullptr;
  }
  return it->second();
}

std::vector<std::string> EffectRegistry::registeredTokens() const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<std::string> tokens;
  tokens.reserve(factories_.size());
  for (const auto& entry : factories_) {
    tokens.push_back(entry.first);
  }
  return tokens;
}

}  // namespace avs::effects
