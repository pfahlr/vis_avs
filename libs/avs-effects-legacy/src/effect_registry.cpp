#include "avs/effects/effect_registry.hpp"
static std::unordered_map<std::string, EffectFactory> g_reg;
void RegisterEffectFactory(const std::string& token, EffectFactory f) { g_reg[token] = std::move(f); }
const std::unordered_map<std::string, EffectFactory>& GetEffectRegistry() { return g_reg; }
