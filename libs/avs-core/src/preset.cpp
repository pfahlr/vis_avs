#include "avs/preset.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

namespace avs {

namespace {
std::string trim(const std::string& s) {
  size_t b = 0;
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
  size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
  return s.substr(b, e - b);
}

class PassThroughEffect : public Effect {
 public:
  void process(const Framebuffer& in, Framebuffer& out) override { out = in; }
};

constexpr const char kMagic[] = "Nullsoft AVS Preset 0.2\x1a";

struct Reader {
  const std::vector<char>& data;
  size_t pos = 0;

  size_t remaining() const { return data.size() - pos; }
};

bool readByte(Reader& r, std::uint8_t& value) {
  if (r.pos >= r.data.size()) return false;
  value = static_cast<std::uint8_t>(r.data[r.pos++]);
  return true;
}

bool readU32(Reader& r, std::uint32_t& value) {
  if (r.pos + 4 > r.data.size()) return false;
  value = static_cast<std::uint32_t>(static_cast<std::uint8_t>(r.data[r.pos])) |
          (static_cast<std::uint32_t>(static_cast<std::uint8_t>(r.data[r.pos + 1])) << 8) |
          (static_cast<std::uint32_t>(static_cast<std::uint8_t>(r.data[r.pos + 2])) << 16) |
          (static_cast<std::uint32_t>(static_cast<std::uint8_t>(r.data[r.pos + 3])) << 24);
  r.pos += 4;
  return true;
}

bool parseColorModifier(Reader& r, size_t len, ParsedPreset& result) {
  if (r.remaining() < len) return false;
  size_t chunkEnd = r.pos + len;
  std::uint8_t version = 0;
  if (!readByte(r, version)) return false;
  if (version != 1) {
    r.pos = chunkEnd;
    return false;
  }
  std::array<std::string, 4> scripts;
  for (size_t i = 0; i < scripts.size(); ++i) {
    std::uint32_t slen = 0;
    if (!readU32(r, slen)) {
      r.pos = chunkEnd;
      return false;
    }
    if (slen > chunkEnd - r.pos) {
      r.pos = chunkEnd;
      return false;
    }
    if (slen > 0) {
      const char* begin = r.data.data() + r.pos;
      scripts[i].assign(begin, begin + slen);
      if (!scripts[i].empty() && scripts[i].back() == '\0') scripts[i].pop_back();
      r.pos += slen;
    } else {
      scripts[i].clear();
    }
  }
  std::uint32_t recompute = 0;
  if (!readU32(r, recompute)) {
    r.pos = chunkEnd;
    return false;
  }
  r.pos = chunkEnd;
  result.chain.push_back(
      std::make_unique<ScriptedEffect>(scripts[3], scripts[1], scripts[2], scripts[0]));
  return true;
}

ParsedPreset parseBinaryPreset(const std::vector<char>& data) {
  ParsedPreset result;
  Reader r{data, sizeof(kMagic) - 1};
  std::uint8_t modeByte = 0;
  if (!readByte(r, modeByte)) {
    result.warnings.push_back("incomplete preset header");
    return result;
  }
  std::uint32_t mode = modeByte;
  if (modeByte & 0x80u) {
    std::uint32_t ext = 0;
    if (!readU32(r, ext)) {
      result.warnings.push_back("corrupt preset mode");
      return result;
    }
    mode = (modeByte & ~0x80u) | ext;
  }
  std::uint32_t extendedSize = (mode >> 24) & 0xFFu;
  size_t skip = extendedSize ? static_cast<size_t>(extendedSize) + 4u : 0u;
  if (skip > 0) {
    if (r.remaining() < skip) {
      result.warnings.push_back("truncated extended preset data");
      return result;
    }
    r.pos += skip;
  }
  while (r.remaining() >= 8) {
    std::uint32_t effectId = 0;
    std::uint32_t payloadLen = 0;
    if (!readU32(r, effectId) || !readU32(r, payloadLen)) break;
    if (r.remaining() < payloadLen) {
      result.warnings.push_back("truncated effect payload");
      break;
    }
    size_t chunkStart = r.pos;
    bool handled = false;
    if (effectId == 45u) {
      Reader chunkReader{r.data, r.pos};
      handled = parseColorModifier(chunkReader, payloadLen, result);
    }
    if (!handled) {
      result.warnings.push_back("unsupported effect index: " + std::to_string(effectId));
      result.chain.push_back(std::make_unique<PassThroughEffect>());
      result.unknown.push_back("effect:" + std::to_string(effectId));
    }
    r.pos = chunkStart + payloadLen;
  }
  return result;
}

ParsedPreset parseTextPreset(const std::string& text) {
  ParsedPreset result;
  std::istringstream stream(text);
  std::string line;
  while (std::getline(stream, line)) {
    std::string t = trim(line);
    if (t.empty() || t[0] == '#') continue;
    std::istringstream iss(t);
    std::string type;
    iss >> type;
    if (type == "blur") {
      int radius = 5;
      std::string token;
      while (iss >> token) {
        if (token.rfind("radius=", 0) == 0) {
          radius = std::stoi(token.substr(7));
        } else {
          result.unknown.push_back(token);
        }
      }
      result.chain.push_back(std::make_unique<BlurEffect>(radius));
    } else if (type == "colormap") {
      result.chain.push_back(std::make_unique<ColorMapEffect>());
    } else if (type == "convolution") {
      result.chain.push_back(std::make_unique<ConvolutionEffect>());
    } else if (type == "scripted") {
      std::string script;
      std::getline(iss, script);
      script = trim(script);
      result.chain.push_back(std::make_unique<ScriptedEffect>("", script, "", ""));
    } else {
      result.warnings.push_back("unsupported effect: " + type);
      result.chain.push_back(std::make_unique<PassThroughEffect>());
      result.unknown.push_back(t);
    }
  }
  return result;
}

}  // namespace

ParsedPreset parsePreset(const std::filesystem::path& file) {
  ParsedPreset result;
  std::ifstream f(file, std::ios::binary);
  if (!f) {
    result.warnings.push_back("failed to open preset");
    return result;
  }
  std::vector<char> buffer((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  if (buffer.size() >= sizeof(kMagic) - 1 &&
      std::equal(kMagic, kMagic + sizeof(kMagic) - 1, buffer.begin())) {
    return parseBinaryPreset(buffer);
  }
  return parseTextPreset(std::string(buffer.begin(), buffer.end()));
}

}  // namespace avs
