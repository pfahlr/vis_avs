#pragma once
#include <avs/compat/effects.hpp>
#include <avs/compat/preset.hpp>

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

namespace avs::effects::legacy {

using EffectFactory =
    std::function<std::unique_ptr<avs::Effect>(const LegacyEffectEntry&, ParsedPreset&)>;

void RegisterEffectFactory(std::string token, EffectFactory factory);
const std::unordered_map<std::string, EffectFactory>& GetEffectRegistry();
std::string normalizeLegacyToken(std::string_view token);

}  // namespace avs::effects::legacy

#define AVS_LEGACY_CONCAT_IMPL(a, b) a##b
#define AVS_LEGACY_CONCAT(a, b) AVS_LEGACY_CONCAT_IMPL(a, b)

#define AVS_LEGACY_REGISTER_EFFECT(TOKEN, FACTORY) \
  AVS_LEGACY_REGISTER_EFFECT_IMPL(TOKEN, FACTORY, __COUNTER__)

#define AVS_LEGACY_REGISTER_EFFECT_IMPL(TOKEN, FACTORY, COUNT)                                     \
  namespace {                                                                                     \
  struct AVS_LEGACY_CONCAT(AvsLegacyAutoReg_, COUNT) {                                            \
    AVS_LEGACY_CONCAT(AvsLegacyAutoReg_, COUNT)() {                                               \
      ::avs::effects::legacy::RegisterEffectFactory(TOKEN, FACTORY);                              \
    }                                                                                             \
  };                                                                                               \
  static AVS_LEGACY_CONCAT(AvsLegacyAutoReg_, COUNT)                                              \
      AVS_LEGACY_CONCAT(gAvsLegacyAutoRegInstance_, COUNT);                                       \

}
