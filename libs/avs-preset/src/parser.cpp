#include <avs/effects/registry.h>
#include <avs/preset/parser.h>

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

namespace {

std::string trimCopy(std::string_view value) {
  size_t begin = 0;
  size_t end = value.size();
  while (begin < end && std::isspace(static_cast<unsigned char>(value[begin]))) {
    ++begin;
  }
  while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
    --end;
  }
  return std::string(value.substr(begin, end - begin));
}

bool iequals(std::string_view lhs, std::string_view rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (size_t i = 0; i < lhs.size(); ++i) {
    if (std::tolower(static_cast<unsigned char>(lhs[i])) !=
        std::tolower(static_cast<unsigned char>(rhs[i]))) {
      return false;
    }
  }
  return true;
}

bool parseBool(std::string_view value, bool& out) {
  if (iequals(value, "true") || iequals(value, "yes") || iequals(value, "on")) {
    out = true;
    return true;
  }
  if (iequals(value, "false") || iequals(value, "no") || iequals(value, "off")) {
    out = false;
    return true;
  }
  return false;
}

bool parseInt(std::string_view value, int& out) {
  if (value.empty()) {
    return false;
  }
  size_t pos = 0;
  if (value[pos] == '+' || value[pos] == '-') {
    ++pos;
  }
  if (pos >= value.size() || !std::isdigit(static_cast<unsigned char>(value[pos]))) {
    return false;
  }
  for (size_t i = pos; i < value.size(); ++i) {
    if (!std::isdigit(static_cast<unsigned char>(value[i]))) {
      return false;
    }
  }
  std::string copy(value);
  char* end = nullptr;
  errno = 0;
  long parsed = std::strtol(copy.c_str(), &end, 10);
  if (errno != 0 || end != copy.c_str() + copy.size()) {
    return false;
  }
  out = static_cast<int>(parsed);
  return true;
}

bool parseFloat(std::string_view value, float& out) {
  if (value.empty()) {
    return false;
  }
  std::string copy(value);
  char* end = nullptr;
  errno = 0;
  float parsed = std::strtof(copy.c_str(), &end);
  if (errno != 0 || end != copy.c_str() + copy.size()) {
    return false;
  }
  out = parsed;
  return true;
}

std::string stripQuotes(std::string value) {
  if (value.size() >= 2 && ((value.front() == '"' && value.back() == '"') ||
                            (value.front() == '\'' && value.back() == '\''))) {
    return value.substr(1, value.size() - 2);
  }
  return value;
}

avs::preset::IRParam makeParam(std::string name, std::string value) {
  avs::preset::IRParam param;
  param.name = std::move(name);
  const std::string trimmedValue = trimCopy(value);
  if (trimmedValue.empty()) {
    param.kind = avs::preset::IRParam::STR;
    param.s = "";
    return param;
  }
  bool boolValue = false;
  if (parseBool(trimmedValue, boolValue)) {
    param.kind = avs::preset::IRParam::BOOL;
    param.b = boolValue;
    param.i = boolValue ? 1 : 0;
    param.f = boolValue ? 1.0f : 0.0f;
    return param;
  }
  int intValue = 0;
  if (parseInt(trimmedValue, intValue)) {
    param.kind = avs::preset::IRParam::I32;
    param.i = intValue;
    param.f = static_cast<float>(intValue);
    param.b = intValue != 0;
    return param;
  }
  float floatValue = 0.0f;
  if (parseFloat(trimmedValue, floatValue)) {
    param.kind = avs::preset::IRParam::F32;
    param.f = floatValue;
    return param;
  }
  param.kind = avs::preset::IRParam::STR;
  param.s = stripQuotes(trimmedValue);
  return param;
}

void parseParamList(std::string_view section, std::vector<avs::preset::IRParam>& out) {
  size_t pos = 0;
  while (pos < section.size()) {
    // Find next delimiter, but respect quotes
    bool inQuote = false;
    bool escape = false;
    size_t next = std::string_view::npos;

    for (size_t i = pos; i < section.size(); ++i) {
      char c = section[i];

      if (escape) {
        escape = false;
        continue;
      }

      if (c == '\\') {
        escape = true;
        continue;
      }

      if (c == '"') {
        inQuote = !inQuote;
        continue;
      }

      if (!inQuote && (c == ';' || c == ',')) {
        next = i;
        break;
      }
    }

    std::string_view part =
        section.substr(pos, next == std::string_view::npos ? std::string_view::npos : next - pos);
    pos = (next == std::string_view::npos) ? section.size() : next + 1;
    const std::string trimmed = trimCopy(part);
    if (trimmed.empty()) {
      continue;
    }
    size_t eq = trimmed.find('=');
    if (eq == std::string::npos) {
      out.push_back(makeParam(trimmed, "true"));
      continue;
    }
    std::string name = trimCopy(std::string_view(trimmed).substr(0, eq));
    std::string value = trimCopy(std::string_view(trimmed).substr(eq + 1));
    out.push_back(makeParam(std::move(name), std::move(value)));
  }
}

}  // namespace

namespace avs::preset {

IRPreset parse_legacy_preset(std::string_view text) {
  IRPreset preset;
  preset.compat = "strict";
  int order = 0;
  size_t pos = 0;
  while (pos <= text.size()) {
    size_t newline = text.find_first_of("\r\n", pos);
    std::string_view line = text.substr(
        pos, newline == std::string_view::npos ? std::string_view::npos : newline - pos);
    if (newline == std::string_view::npos) {
      pos = text.size() + 1;
    } else {
      pos = newline + 1;
      if (newline + 1 < text.size() && text[newline] == '\r' && text[newline + 1] == '\n') {
        ++pos;
      }
    }
    const std::string trimmed = trimCopy(line);
    if (trimmed.empty()) {
      continue;
    }
    if (trimmed.front() == '#' || trimmed.front() == ';') {
      continue;
    }
    std::string token = trimmed;
    std::string paramsSection;
    size_t pipe = trimmed.find('|');
    if (pipe != std::string::npos) {
      token = trimCopy(std::string_view(trimmed).substr(0, pipe));
      paramsSection = trimCopy(std::string_view(trimmed).substr(pipe + 1));
    }
    if (token.empty()) {
      continue;
    }
    IRNode node;
    node.token = token;
    node.order_index = order++;
    if (!paramsSection.empty()) {
      parseParamList(paramsSection, node.params);
    }
    preset.root_nodes.push_back(std::move(node));
  }
  return preset;
}

}  // namespace avs::preset

namespace avs::runtime::parser {

std::string normalizeEffectToken(std::string_view token) {
  return avs::effects::Registry::normalize_legacy_token(token);
}

}  // namespace avs::runtime::parser
