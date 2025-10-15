#include "avs/core/EffectRegistry.hpp"

#include <utility>

namespace avs::core {

bool EffectRegistry::registerFactory(std::string key, Factory factory) {
  if (key.empty() || !factory) {
    return false;
  }
  auto [it, inserted] = factories_.emplace(std::move(key), std::move(factory));
  if (!inserted) {
    return false;
  }
  return true;
}

std::unique_ptr<IEffect> EffectRegistry::make(const std::string& key) const {
  auto it = factories_.find(key);
  if (it == factories_.end()) {
    return nullptr;
  }
  return it->second ? it->second() : nullptr;
}

}  // namespace avs::core
