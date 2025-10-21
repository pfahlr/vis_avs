#include "runtime/parser.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <unordered_map>

namespace {

std::string toLowerCopy(std::string_view value) {
  std::string result(value);
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  return result;
}

const std::unordered_map<std::string, std::string>& stubEffectTokens() {
  static const std::unordered_map<std::string, std::string> tokens = {
      {"channel shift", "channel shift"},
      {"color reduction", "color reduction"},
      {"holden04: video delay", "holden04: video delay"},
      {"holden05: multi delay", "holden05: multi delay"},
      {"misc / comment", "misc / comment"},
      {"misc / custom bpm", "misc / custom bpm"},
      {"misc / set render mode", "misc / set render mode"},
      {"multiplier", "multiplier"},
      {"render / avi", "render / avi"},
      {"render / bass spin", "render / bass spin"},
      {"render / dot fountain", "render / dot fountain"},
      {"render / moving particle", "render / moving particle"},
      {"render / oscilloscope star", "render / oscilloscope star"},
      {"render / ring", "render / ring"},
      {"render / rotating stars", "render / rotating stars"},
      {"render / simple", "render / simple"},
      {"render / svp loader", "render / svp loader"},
      {"render / timescope", "render / timescope"},
      {"trans / blitter feedback", "trans / blitter feedback"},
      {"trans / blur", "trans / blur"},
      {"trans / brightness", "trans / brightness"},
      {"trans / color clip", "trans / color clip"},
      {"trans / color modifier", "trans / color modifier"},
      {"trans / colorfade", "trans / colorfade"},
      {"trans / mosaic", "trans / mosaic"},
      {"trans / roto blitter", "trans / roto blitter"},
      {"trans / scatter", "trans / scatter"},
      {"trans / unique tone", "trans / unique tone"},
      {"trans / water", "trans / water"},
      {"trans / water bump", "trans / water bump"},
  };
  return tokens;
}

}  // namespace

namespace avs::runtime::parser {

std::string normalizeEffectToken(std::string_view token) {
  const std::string lowered = toLowerCopy(token);
  const auto& tokens = stubEffectTokens();
  if (auto it = tokens.find(lowered); it != tokens.end()) {
    return it->second;
  }
  return lowered;
}

}  // namespace avs::runtime::parser
