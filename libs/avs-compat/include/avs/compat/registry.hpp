#pragma once
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>
#include "effect.hpp"

namespace avs {

class Registry {
public:
  void register_effect(const EffectDescriptor& d) {
    effects_.push_back(d);
    by_id_[d.id] = (int)effects_.size() - 1;
  }

  const std::vector<EffectDescriptor>& effects() const { return effects_; }

  std::unique_ptr<IEffect> create(std::string_view id) const;

private:
  std::vector<EffectDescriptor> effects_;
  std::unordered_map<std::string, int> by_id_;
};

// Declaration: defined in your .cpp to populate all built-ins.
void register_builtin_effects(Registry& r);

} // namespace avs
