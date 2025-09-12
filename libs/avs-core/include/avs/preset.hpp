#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "avs/effects.hpp"

namespace avs {

struct ParsedPreset {
  std::vector<std::unique_ptr<Effect>> chain;
  std::vector<std::string> warnings;
  std::vector<std::string> unknown;
};

ParsedPreset parsePreset(const std::filesystem::path& file);

}  // namespace avs
