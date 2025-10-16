#pragma once

#include <string>

#include "avs/core/IEffect.hpp"
#include "avs/core/ParamBlock.hpp"

// Base class for compatibility stub effects originating from legacy AVS modules.
class Effect : public avs::core::IEffect {
 public:
  explicit Effect(std::string displayName);
  ~Effect() override = default;

  void setParams(const avs::core::ParamBlock& params) override;

 protected:
  [[nodiscard]] const std::string& displayName() const noexcept { return displayName_; }
  [[nodiscard]] const avs::core::ParamBlock& params() const noexcept { return params_; }

 private:
  std::string displayName_;
  avs::core::ParamBlock params_;
};
