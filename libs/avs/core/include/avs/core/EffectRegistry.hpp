#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "avs/core/IEffect.hpp"

namespace avs::core {

/**
 * @brief Registry mapping effect keys to factory functions.
 */
class EffectRegistry {
 public:
  using Factory = std::function<std::unique_ptr<IEffect>()>;

  /**
   * @brief Register a factory for the supplied key.
   * @return true when the key was inserted, false for duplicates or invalid input.
   */
  bool registerFactory(std::string key, Factory factory);

  /**
   * @brief Construct a new effect instance from a previously registered key.
   */
  [[nodiscard]] std::unique_ptr<IEffect> make(const std::string& key) const;

 private:
  std::unordered_map<std::string, Factory> factories_;
};

}  // namespace avs::core
