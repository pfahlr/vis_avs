#pragma once
#include <avs/effects/graph.h>
#include <avs/effects/registry.h>
#include <avs/preset/ir.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace avs::preset {

struct BindOptions {
  std::string compat = "strict";
};

std::unique_ptr<avs::effects::Graph> bind_preset(const IRPreset& ir, const BindOptions& opt,
                                                 avs::effects::Registry& reg);

}  // namespace avs::preset
