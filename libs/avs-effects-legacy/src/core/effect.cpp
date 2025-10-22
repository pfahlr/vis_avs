#include <avs/effects/legacy/effect.h>

#include <iostream>
#include <utility>

Effect::Effect(std::string displayName) : displayName_(std::move(displayName)) {
  std::clog << "Loaded stub: " << displayName_ << " — behavior not implemented" << std::endl;
}

void Effect::setParams(const avs::core::ParamBlock& params) { params_ = params; }
