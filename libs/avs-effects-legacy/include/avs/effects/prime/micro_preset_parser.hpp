#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <avs/core/ParamBlock.hpp>

namespace avs::effects {

struct MicroEffectCommand {
  std::string effectKey;
  avs::core::ParamBlock params;
};

struct MicroPreset {
  std::vector<MicroEffectCommand> commands;
  std::vector<std::string> warnings;
};

MicroPreset parseMicroPreset(std::string_view text);

}  // namespace avs::effects
