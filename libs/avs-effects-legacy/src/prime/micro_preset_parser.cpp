#include <avs/effects/prime/micro_preset_parser.hpp>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

#include <avs/effects/registry.h>

namespace {

std::string trimCopy(std::string_view text) {
  std::size_t begin = 0;
  while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin]))) {
    ++begin;
  }
  std::size_t end = text.size();
  while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
    --end;
  }
  return std::string(text.substr(begin, end - begin));
}

std::string toLower(std::string_view text) {
  std::string result(text);
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return result;
}

std::string toUpper(std::string_view text) {
  std::string result(text);
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
  return result;
}

struct UiPattern {
  std::string token;
  bool prefix;
};

bool isUiToken(const std::string& tokenUpper) {
  static const UiPattern patterns[] = {
      {"BUTTON", true},       {"CHECK", true},         {"EDIT", true},
      {"SLIDER", true},       {"RADIO", true},         {"TAB1", false},
      {"LIST1", false},       {"SCROLLBAR1", false},   {"HELPBTN", false},
      {"CHOOSEFONT", false},  {"VIS_", true},          {"L_", true},
      {"DEBUGREG_", true},    {"EFFECTRECT", false},   {"EFFECTS", false},
      {"EFNAME", false},      {"SETTINGS", false},     {"VERSTR", false},
      {"TRANS_CHECK", false}, {"TRANS_SLIDER", false}, {"THREADSBORDER", false},
      {"REMSEL", false},      {"EXCLUDE", false},      {"NEWRESET", false},
      {"HRESET", false},      {"VRESET", false},       {"MAX", false},
      {"OFF", false},         {"IN", false},           {"OUT", false},
      {"SA", false},          {"QUAL", true},
  };
  for (const auto& pattern : patterns) {
    if (pattern.prefix) {
      if (tokenUpper.rfind(pattern.token, 0) == 0) {
        return true;
      }
    } else if (tokenUpper == pattern.token) {
      return true;
    }
  }
  return false;
}

std::vector<std::string> tokenize(std::string_view line) {
  std::vector<std::string> tokens;
  std::string current;
  bool inQuote = false;
  char quoteChar = '\0';
  for (char ch : line) {
    if (inQuote) {
      if (ch == quoteChar) {
        tokens.push_back(current);
        current.clear();
        inQuote = false;
      } else {
        current.push_back(ch);
      }
    } else if (std::isspace(static_cast<unsigned char>(ch))) {
      if (!current.empty()) {
        tokens.push_back(current);
        current.clear();
      }
    } else if (ch == '"' || ch == 39) {
      if (!current.empty()) {
        tokens.push_back(current);
        current.clear();
      }
      inQuote = true;
      quoteChar = ch;
    } else {
      current.push_back(ch);
    }
  }
  if (!current.empty()) {
    tokens.push_back(current);
  }
  return tokens;
}

bool tryParseInt(const std::string& value, int base, int& out) {
  const char* begin = value.c_str();
  const char* end = begin + value.size();
  std::from_chars_result result = std::from_chars(begin, end, out, base);
  return result.ec == std::errc() && result.ptr == end;
}

void assignValue(avs::core::ParamBlock& params, const std::string& key, const std::string& value) {
  if (value.empty()) {
    params.setBool(key, true);
    return;
  }
  const std::string lowerValue = toLower(value);
  if (lowerValue == "true" || lowerValue == "on" || lowerValue == "yes") {
    params.setBool(key, true);
    return;
  }
  if (lowerValue == "false" || lowerValue == "off" || lowerValue == "no") {
    params.setBool(key, false);
    return;
  }

  std::string numeric = value;
  int base = 10;
  if (!numeric.empty() && numeric.front() == '#') {
    numeric = numeric.substr(1);
    base = 16;
  } else if (numeric.size() > 2 && numeric[0] == '0' && (numeric[1] == 'x' || numeric[1] == 'X')) {
    numeric = numeric.substr(2);
    base = 16;
  }

  int parsedInt = 0;
  if (!numeric.empty() && tryParseInt(numeric, base, parsedInt)) {
    params.setInt(key, parsedInt);
    return;
  }

  if (value.find('.') != std::string::npos) {
    try {
      float parsedFloat = std::stof(value);
      params.setFloat(key, parsedFloat);
      return;
    } catch (...) {
      // Fall through to string assignment.
    }
  }

  params.setString(key, value);
}

}  // namespace

namespace avs::effects {

MicroPreset parseMicroPreset(std::string_view text) {
  MicroPreset preset;
  std::istringstream stream{std::string(text)};
  std::string line;
  while (std::getline(stream, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    bool inQuote = false;
    char quoteChar = '\0';
    for (std::size_t i = 0; i < line.size(); ++i) {
      const char ch = line[i];
      if (inQuote) {
        if (ch == quoteChar) {
          inQuote = false;
        }
        continue;
      }
      if (ch == '"' || ch == 39) {
        inQuote = true;
        quoteChar = ch;
        continue;
      }
      if (ch == '#') {
        std::size_t pos = i;
        while (pos > 0) {
          const unsigned char prev = static_cast<unsigned char>(line[pos - 1]);
          if (!std::isspace(prev)) {
            break;
          }
          --pos;
        }
        bool treatAsComment = true;
        if (pos > 0) {
          const unsigned char prev = static_cast<unsigned char>(line[pos - 1]);
          if (prev == '=') {
            treatAsComment = false;
          }
        }
        if (treatAsComment) {
          line.erase(i);
          break;
        }
      }
    }
    const std::string trimmed = trimCopy(line);
    if (trimmed.empty()) {
      continue;
    }
    const auto tokens = tokenize(trimmed);
    if (tokens.empty()) {
      continue;
    }
    const std::string effectToken = tokens.front();
    const std::string effectUpper = toUpper(effectToken);
    if (isUiToken(effectUpper)) {
      preset.warnings.push_back("ignored token: " + effectToken);
      continue;
    }
    MicroEffectCommand command;
    command.effectKey = avs::effects::Registry::normalize_legacy_token(effectToken);
    const bool isCommentEffect =
        command.effectKey == "misc_comment" || command.effectKey == "misc / comment";
    auto isCommentKey = [](const std::string& key) {
      return key == "comment" || key == "text" || key == "message" || key == "msg";
    };
    std::string explicitComment;
    std::string inlineComment;
    bool commentKeySeen = false;
    bool awaitingCommentValue = false;
    for (std::size_t i = 1; i < tokens.size(); ++i) {
      const std::string& token = tokens[i];
      if (isCommentEffect && awaitingCommentValue) {
        if (!explicitComment.empty()) {
          explicitComment.push_back(' ');
        }
        explicitComment += token;
        awaitingCommentValue = false;
        continue;
      }
      const auto eqPos = token.find('=');
      if (eqPos == std::string::npos) {
        if (isCommentEffect) {
          if (!inlineComment.empty()) {
            inlineComment.push_back(' ');
          }
          inlineComment += token;
        } else {
          command.params.setBool(toLower(token), true);
        }
        continue;
      }
      std::string key = toLower(token.substr(0, eqPos));
      std::string value = token.substr(eqPos + 1);
      if (isCommentEffect) {
        if (isCommentKey(key)) {
          commentKeySeen = true;
          if (!value.empty()) {
            if (!explicitComment.empty()) {
              explicitComment.push_back(' ');
            }
            explicitComment += value;
          } else {
            awaitingCommentValue = true;
          }
          continue;
        }
        if (!inlineComment.empty()) {
          inlineComment.push_back(' ');
        }
        inlineComment += token;
        continue;
      }
      assignValue(command.params, key, value);
    }
    if (isCommentEffect) {
      std::string combined = explicitComment;
      if (awaitingCommentValue && commentKeySeen && combined.empty()) {
        combined.clear();
      }
      if (!inlineComment.empty()) {
        if (!combined.empty()) {
          combined.push_back(' ');
        }
        combined += inlineComment;
      }
      if (commentKeySeen || !combined.empty()) {
        command.params.setString("comment", combined);
      }
    }
    preset.commands.push_back(std::move(command));
  }
  return preset;
}

}  // namespace avs::effects
