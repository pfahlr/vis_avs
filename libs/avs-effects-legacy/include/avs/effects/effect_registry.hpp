#pragma once
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>

// Every effect subclass implements IEffect (or your existing base)
struct IEffect { virtual ~IEffect() = default; /* process(), etc. */ };

using EffectFactory = std::function<std::unique_ptr<IEffect>()>;
const std::unordered_map<std::string, EffectFactory>& GetEffectRegistry();

// Helper macro each effect uses exactly once, with canonical token:
#define AVS_EFFECT_TOKEN(TOK) \
  static const char* kAvsEffectToken() { return TOK; }

#define REGISTER_AVS_EFFECT(CLASS, TOKEN) \
  namespace { \
    struct CLASS##__AutoReg { \
      CLASS##__AutoReg() { \
        RegisterEffectFactory(TOKEN, [](){ return std::make_unique<CLASS>(); }); \
      } \
    }; \
    static CLASS##__AutoReg g_##CLASS##_AutoReg; \
  }
void RegisterEffectFactory(const std::string& token, EffectFactory f);
