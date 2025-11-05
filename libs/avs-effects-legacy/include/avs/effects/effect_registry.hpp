#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "avs/effects/legacy_effect.h"

namespace avs::effects {

class EffectRegistry {
 public:
  using Factory = std::function<std::unique_ptr<LegacyEffect>()>;

  static EffectRegistry& instance();

  bool registerEffect(const std::string& token, Factory factory);

  std::unique_ptr<LegacyEffect> create(const std::string& token) const;

  std::vector<std::string> registeredTokens() const;

 private:
  EffectRegistry() = default;

  using FactoryMap = std::unordered_map<std::string, Factory>;
  mutable std::mutex mutex_;
  FactoryMap factories_;
};

}  // namespace avs::effects

#define AVS_EFFECT_TOKEN(token_literal)                        \
  namespace {                                                  \
  [[maybe_unused]] constexpr const char kAvsEffectToken[] = token_literal; \
  }

#define REGISTER_AVS_EFFECT(ClassName, token_literal)                                    \
  namespace {                                                                            \
  [[maybe_unused]] const bool kAvsEffectRegistered_##ClassName =                         \
      ::avs::effects::EffectRegistry::instance().registerEffect(                         \
          token_literal, []() { return std::make_unique<ClassName>(); });                 \
  }
