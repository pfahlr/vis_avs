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
constexpr std::uint32_t kListId = 0xFFFFFFFEu;

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

bool readByteBounded(Reader& r, size_t limit, std::uint8_t& value) {
  if (r.pos >= limit) return false;
  return readByte(r, value);
}

bool readU32Bounded(Reader& r, size_t limit, std::uint32_t& value) {
  if (r.pos + 4 > limit) return false;
  return readU32(r, value);
}

bool ensureRemaining(const Reader& r, size_t limit, size_t amount) {
  return r.pos + amount <= limit;
}

bool parseRenderListChunk(Reader& r,
                          size_t chunkEnd,
                          ParsedPreset& result,
                          std::vector<std::unique_ptr<Effect>>& chain);

bool parseColorModifier(Reader& r, size_t chunkEnd, std::unique_ptr<Effect>& effect) {
  std::uint8_t version = 0;
  if (!readByteBounded(r, chunkEnd, version)) return false;
  if (version != 1) {
    r.pos = chunkEnd;
    return false;
  }
  std::array<std::string, 4> scripts;
  for (size_t i = 0; i < scripts.size(); ++i) {
    std::uint32_t slen = 0;
    if (!readU32Bounded(r, chunkEnd, slen)) {
      r.pos = chunkEnd;
      return false;
    }
    size_t remaining = chunkEnd - r.pos;
    if (slen > remaining) {
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
  if (!readU32Bounded(r, chunkEnd, recompute)) {
    r.pos = chunkEnd;
    return false;
  }
  r.pos = chunkEnd;
  effect = std::make_unique<ScriptedEffect>(scripts[3], scripts[1], scripts[2], scripts[0]);
  return true;
}

bool parseCommentEffect(Reader& r, size_t chunkEnd, ParsedPreset& result) {
  std::uint32_t rawLen = 0;
  if (!readU32Bounded(r, chunkEnd, rawLen)) {
    r.pos = chunkEnd;
    return false;
  }
  size_t strLen = static_cast<size_t>(rawLen);
  if (strLen > chunkEnd - r.pos) {
    r.pos = chunkEnd;
    return false;
  }
  std::string text;
  if (strLen > 0) {
    const char* begin = r.data.data() + r.pos;
    text.assign(begin, begin + strLen);
    if (!text.empty() && text.back() == '\0') text.pop_back();
    r.pos += strLen;
  }
  result.comments.push_back(std::move(text));
  r.pos = chunkEnd;
  return true;
}

bool parseRenderListChunk(Reader& r,
                          size_t chunkEnd,
                          ParsedPreset& result,
                          std::vector<std::unique_ptr<Effect>>& chain) {
  if (chunkEnd > r.data.size()) {
    result.warnings.push_back("render list exceeds buffer bounds");
    r.pos = r.data.size();
    return false;
  }
  if (r.pos >= chunkEnd) return true;

  std::uint8_t modeByte = 0;
  if (!readByteBounded(r, chunkEnd, modeByte)) {
    result.warnings.push_back("incomplete preset header");
    r.pos = chunkEnd;
    return false;
  }
  std::uint32_t mode = modeByte;
  if (modeByte & 0x80u) {
    std::uint32_t ext = 0;
    if (!readU32Bounded(r, chunkEnd, ext)) {
      result.warnings.push_back("corrupt preset mode");
      r.pos = chunkEnd;
      return false;
    }
    mode = (modeByte & ~0x80u) | ext;
  }
  std::uint32_t extendedSize = (mode >> 24) & 0xFFu;
  size_t skip = extendedSize ? static_cast<size_t>(extendedSize) + 4u : 0u;
  if (skip > 0) {
    if (!ensureRemaining(r, chunkEnd, skip)) {
      result.warnings.push_back("truncated extended preset data");
      r.pos = chunkEnd;
      return false;
    }
    r.pos += skip;
  }

  while (ensureRemaining(r, chunkEnd, 8)) {
    std::uint32_t effectId = 0;
    std::uint32_t payloadLen = 0;
    if (!readU32Bounded(r, chunkEnd, effectId) || !readU32Bounded(r, chunkEnd, payloadLen)) {
      result.warnings.push_back("truncated effect header");
      r.pos = chunkEnd;
      return false;
    }
    size_t payloadStart = r.pos;
    size_t payloadEnd = payloadStart + static_cast<size_t>(payloadLen);
    if (payloadEnd > chunkEnd || payloadEnd > r.data.size()) {
      result.warnings.push_back("truncated effect payload");
      r.pos = chunkEnd;
      return false;
    }

    Reader chunkReader{r.data, payloadStart};
    std::unique_ptr<Effect> parsedEffect;
    bool knownEffect = false;
    bool success = false;

    if (effectId == 45u) {
      knownEffect = true;
      success = parseColorModifier(chunkReader, payloadEnd, parsedEffect);
    } else if (effectId == 21u) {
      knownEffect = true;
      success = parseCommentEffect(chunkReader, payloadEnd, result);
    } else if (effectId == kListId) {
      knownEffect = true;
      std::vector<std::unique_ptr<Effect>> nestedChain;
      success = parseRenderListChunk(chunkReader, payloadEnd, result, nestedChain);
      if (success) {
        auto composite = std::make_unique<CompositeEffect>();
        for (auto& child : nestedChain) {
          composite->addEffect(std::move(child));
        }
        parsedEffect = std::move(composite);
      }
    }

    if (knownEffect) {
      if (success) {
        if (parsedEffect) {
          chain.push_back(std::move(parsedEffect));
        }
      } else {
        result.warnings.push_back("failed to parse effect index: " + std::to_string(effectId));
      }
    } else {
      result.warnings.push_back("unsupported effect index: " + std::to_string(effectId));
      chain.push_back(std::make_unique<PassThroughEffect>());
      result.unknown.push_back("effect:" + std::to_string(effectId));
    }

    r.pos = payloadEnd;
  }

  if (r.pos < chunkEnd) {
    r.pos = chunkEnd;
  }
  return true;
}

ParsedPreset parseBinaryPreset(const std::vector<char>& data) {
  ParsedPreset result;
  Reader r{data, sizeof(kMagic) - 1};
  parseRenderListChunk(r, data.size(), result, result.chain);
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
