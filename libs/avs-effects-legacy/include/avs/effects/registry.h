#pragma once
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include "api.h"

namespace avs::effects {

class Registry {
 public:
  void add(Descriptor d);
  const Descriptor* by_id(std::string_view id) const;
  const Descriptor* by_legacy(std::string_view legacy_token_norm) const;

  std::unique_ptr<IEffect> make(std::string_view token_or_id, const ParamList& params,
                                const BuildCtx& ctx, bool* matched_legacy = nullptr) const;

  static std::string normalize_legacy_token(std::string_view s);

 private:
  std::unordered_map<std::string, Descriptor> byId_;
  std::unordered_map<std::string, std::string> legacy2id_;
};

}  // namespace avs::effects
