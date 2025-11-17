#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <avs/effects.hpp>

namespace avs {

struct LegacyEffectEntry {
  std::uint32_t effectId = 0;
  std::string effectName;
  std::vector<std::uint8_t> payload;
};

struct ParsedPreset {
  std::vector<std::unique_ptr<Effect>> chain;
  std::vector<std::string> warnings;
  std::vector<std::string> unknown;
  std::vector<std::string> comments;
  std::vector<LegacyEffectEntry> effects;
};

ParsedPreset parsePreset(const std::filesystem::path& file);

}  // namespace avs
