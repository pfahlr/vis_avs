#pragma once

#include <memory>
#include <string>
#include <vector>

#include "avs/core/EffectRegistry.hpp"
#include "avs/core/ParamBlock.hpp"

namespace avs::core {

/**
 * @brief Ordered collection of effects executed for each frame.
 */
class Pipeline {
 public:
  explicit Pipeline(EffectRegistry& registry);

  /**
   * @brief Instantiate an effect and append it to the execution chain.
   * @param key Factory key used for lookup in the registry.
   * @param params Parameter block passed to the effect before first render.
   */
  void add(std::string key, ParamBlock params);

  /**
   * @brief Execute all registered effects for the given frame.
   * @return true if every effect reported success.
   */
  bool render(RenderContext& context);

  /**
   * @brief Remove all effects from the pipeline.
   */
  void clear();

 private:
  struct Node {
    std::string key;
    ParamBlock params;
    std::unique_ptr<IEffect> effect;
  };

  EffectRegistry& registry_;
  std::vector<Node> nodes_;
};

}  // namespace avs::core
