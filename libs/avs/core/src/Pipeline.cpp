#include "avs/core/Pipeline.hpp"

#include <utility>

namespace avs::core {

Pipeline::Pipeline(EffectRegistry& registry) : registry_(registry) {}

void Pipeline::add(std::string key, ParamBlock params) {
  auto effect = registry_.make(key);
  if (!effect) {
    return;
  }
  effect->setParams(params);
  nodes_.push_back(Node{std::move(key), std::move(params), std::move(effect)});
}

bool Pipeline::render(RenderContext& context) {
  context.rng.reseed(context.frameIndex);
  bool success = true;
  for (auto& node : nodes_) {
    if (!node.effect) {
      continue;
    }
    if (!node.effect->render(context)) {
      success = false;
    }
  }
  return success;
}

void Pipeline::clear() { nodes_.clear(); }

}  // namespace avs::core
