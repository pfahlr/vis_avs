#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace avs::effects {

struct BuildCtx {
  std::string compat;
};

struct ParamValue {
  std::string name;
  enum Kind { F32, I32, BOOL, STR } kind = F32;
  float f = 0.f;
  int i = 0;
  bool b = false;
  std::string s;
};
using ParamList = std::vector<ParamValue>;

struct IEffect {
  virtual ~IEffect() = default;
  virtual const char* id() const = 0;
};

using FactoryFn = std::function<std::unique_ptr<IEffect>(const ParamList&, const BuildCtx&)>;

struct Descriptor {
  std::string id;
  std::vector<std::string> legacy_tokens;
  FactoryFn factory;
};

}  // namespace avs::effects
