#include <avs/preset.hpp>

#include <avs/effects/effect_registry.hpp>
#include <avs/compat/ape_loader.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace avs {

namespace {
constexpr std::uint32_t kApeIdBase = 16384;  // APE effects have IDs >= this value
constexpr size_t kApeIdLength = 32;          // APE ID string is fixed 32 bytes

std::string trim(const std::string& s) {
  size_t b = 0;
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
  size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
  return s.substr(b, e - b);
}

std::string effectNameForId(std::uint32_t effectId) {
  constexpr std::array<std::string_view, 46> kRegisteredEffectNames = {
      "Render / Simple",            // R_SimpleSpectrum
      "Render / Dot Plane",         // R_DotPlane
      "Render / Oscilloscope Star", // R_OscStars (preferred casing from registry)
      "",                           // R_FadeOut
      "Trans / Blitter Feedback",   // R_BlitterFB
      "",                           // R_NFClear
      "Trans / Blur",               // R_Blur
      "Render / Bass Spin",         // R_BSpin
      "Render / Moving Particle",   // R_Parts
      "Trans / Roto Blitter",       // R_RotBlit
      "Render / SVP Loader",        // R_SVP
      "Trans / Colorfade",          // R_ColorFade
      "Trans / Color Clip",         // R_ContrastEnhance
      "Render / Rotating Stars",    // R_RotStar
      "Render / Ring",              // R_OscRings
      "",                           // R_Trans
      "Trans / Scatter",            // R_Scat
      "",                           // R_DotGrid
      "",                           // R_Stack
      "Render / Dot Fountain",      // R_DotFountain
      "Trans / Water",              // R_Water
      "Misc / Comment",             // R_Comment
      "Trans / Brightness",         // R_Brightness
      "",                           // R_Interleave
      "Trans / Grain",              // R_Grain
      "",                           // R_Clear
      "",                           // R_Mirror
      "",                           // R_StarField
      "",                           // R_Text
      "",                           // R_Bump
      "Trans / Mosaic",             // R_Mosaic
      "Trans / Water Bump",         // R_WaterBump
      "Render / AVI",               // R_AVI
      "Misc / Custom BPM",          // R_Bpm
      "",                           // R_Picture
      "",                           // R_DDM
      "",                           // R_SScope
      "",                           // R_Invert
      "Trans / Unique tone",        // R_Onetone
      "Render / Timescope",         // R_Timescope
      "Misc / Set render mode",     // R_LineMode
      "Trans / Interferences",      // R_Interferences
      "",                           // R_Shift
      "",                           // R_DMove
      "Trans / Fast Brightness",    // R_FastBright
      "Trans / Color Modifier",     // R_DColorMod
  };

  constexpr std::array<std::string_view, 46> kLegacyEffectNames = {
      "Render / Simple",                     // R_SimpleSpectrum
      "Render / Dot Plane",                  // R_DotPlane
      "Render / Oscilliscope Star",          // R_OscStars
      "Trans / Fadeout",                     // R_FadeOut
      "Trans / Blitter Feedback",            // R_BlitterFB
      "Render / OnBeat Clear",               // R_NFClear
      "Trans / Blur",                        // R_Blur
      "Render / Bass Spin",                  // R_BSpin
      "Render / Moving Particle",            // R_Parts
      "Trans / Roto Blitter",                // R_RotBlit
      "Render / SVP Loader",                 // R_SVP
      "Trans / Colorfade",                   // R_ColorFade
      "Trans / Color Clip",                  // R_ContrastEnhance
      "Render / Rotating Stars",             // R_RotStar
      "Render / Ring",                       // R_OscRings
      "Trans / Movement",                    // R_Trans
      "Trans / Scatter",                     // R_Scat
      "Render / Dot Grid",                   // R_DotGrid
      "Misc / Buffer Save",                  // R_Stack
      "Render / Dot Fountain",               // R_DotFountain
      "Trans / Water",                       // R_Water
      "Misc / Comment",                      // R_Comment
      "Trans / Brightness",                  // R_Brightness
      "Trans / Interleave",                  // R_Interleave
      "Trans / Grain",                       // R_Grain
      "Render / Clear screen",               // R_Clear
      "Trans / Mirror",                      // R_Mirror
      "Render / Starfield",                  // R_StarField
      "Render / Text",                       // R_Text
      "Trans / Bump",                        // R_Bump
      "Trans / Mosaic",                      // R_Mosaic
      "Trans / Water Bump",                  // R_WaterBump
      "Render / AVI",                        // R_AVI
      "Misc / Custom BPM",                   // R_Bpm
      "Render / Picture",                    // R_Picture
      "Trans / Dynamic Distance Modifier",   // R_DDM
      "Render / SuperScope",                 // R_SScope
      "Trans / Invert",                      // R_Invert
      "Trans / Unique tone",                 // R_Onetone
      "Render / Timescope",                  // R_Timescope
      "Misc / Set render mode",              // R_LineMode
      "Trans / Interferences",               // R_Interferences
      "Trans / Dynamic Shift",               // R_Shift
      "Trans / Dynamic Movement",            // R_DMove
      "Trans / Fast Brightness",             // R_FastBright
      "Trans / Color Modifier",              // R_DColorMod
  };

  if (effectId < kRegisteredEffectNames.size()) {
    const std::string_view preferred = kRegisteredEffectNames[effectId];
    if (!preferred.empty()) {
      return std::string(preferred);
    }
  }
  if (effectId < kLegacyEffectNames.size()) {
    return std::string(kLegacyEffectNames[effectId]);
  }
  return {};
}

std::string describeEffect(std::uint32_t effectId) {
  std::string name = effectNameForId(effectId);
  if (name.empty()) {
    return std::to_string(effectId);
  }
  return std::to_string(effectId) + " (" + name + ")";
}

constexpr std::string_view kMagicPrefix = "Nullsoft AVS Preset ";
constexpr char kMagicTerminator = '\x1a';
constexpr std::array<std::string_view, 2> kKnownMagicVersions = {"0.2", "0.1"};

bool isKnownMagicVersion(std::string_view version) {
  return std::find(kKnownMagicVersions.begin(), kKnownMagicVersions.end(), version) !=
         kKnownMagicVersions.end();
}
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

  const auto& registry = avs::effects::legacy::GetEffectRegistry();

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
  if (extendedSize > 0) {
    // Legacy Winamp render lists store the extended byte count as "actual payload + 4".
    // See r_list.cpp where set_extended_datasize(36) is accompanied by the comment
    // "size of extended data + 4 cause we fucked up". We previously skipped
    // extendedSize + 4, which marched eight bytes past the first effect header once
    // the extended block was present. Consume the declared fields explicitly so we
    // stay aligned with the payload that follows.
    size_t declaredBytes = extendedSize >= 4u ? static_cast<size_t>(extendedSize - 4u) : 0u;
    if (!ensureRemaining(r, chunkEnd, declaredBytes)) {
      result.warnings.push_back("truncated extended preset data");
      r.pos = chunkEnd;
      return false;
    }
    std::array<std::uint32_t, 8> extFields{};
    size_t valuesToRead = std::min(extFields.size(), declaredBytes / sizeof(std::uint32_t));
    for (size_t i = 0; i < valuesToRead; ++i) {
      if (!readU32Bounded(r, chunkEnd, extFields[i])) {
        result.warnings.push_back("incomplete extended preset data");
        r.pos = chunkEnd;
        return false;
      }
    }
    size_t consumed = valuesToRead * sizeof(std::uint32_t);
    if (declaredBytes > consumed) {
      r.pos += declaredBytes - consumed;
    }
  }

  while (ensureRemaining(r, chunkEnd, 8)) {
    std::uint32_t effectId = 0;
    std::uint32_t payloadLen = 0;
    if (!readU32Bounded(r, chunkEnd, effectId)) {
      result.warnings.push_back("truncated effect header");
      r.pos = chunkEnd;
      return false;
    }

    // APE effects (ID >= 16384) have a 32-byte string identifier after the ID
    // However, special sentinel values like kListId (0xFFFFFFFE) should not be treated as APE effects
    std::string apeId;
    if (effectId >= kApeIdBase && effectId != kListId) {
      if (!ensureRemaining(r, chunkEnd, kApeIdLength)) {
        result.warnings.push_back("truncated APE effect identifier");
        r.pos = chunkEnd;
        return false;
      }
      // Read and null-terminate the APE ID string
      const char* apeIdStart = r.data.data() + r.pos;
      apeId.assign(apeIdStart, apeIdStart + kApeIdLength);
      // Trim to actual string length (null-terminated)
      size_t nullPos = apeId.find('\0');
      if (nullPos != std::string::npos) {
        apeId.resize(nullPos);
      }
      r.pos += kApeIdLength;
    }

    if (!readU32Bounded(r, chunkEnd, payloadLen)) {
      result.warnings.push_back("truncated effect payload length");
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
    bool handledEffect = false;
    bool success = false;

    LegacyEffectEntry entry;
    entry.effectId = effectId;
    // For APE effects, use the APE ID string as the effect name; otherwise use ID lookup
    entry.effectName = !apeId.empty() ? apeId : effectNameForId(effectId);
    if (payloadLen > 0) {
      const auto* payloadPtr = reinterpret_cast<const std::uint8_t*>(r.data.data() + payloadStart);
      entry.payload.assign(payloadPtr, payloadPtr + payloadLen);
    }

    auto makeUnknown = [&](const std::string& token) {
      const std::string label = token.empty() ? describeEffect(effectId) : token;
      result.unknown.push_back("effect:" + label);
      std::fprintf(stderr, "WARNING: Unknown binary effect: %s (ID=%u)\n", label.c_str(), effectId);
      return std::make_unique<UnknownRenderObjectEffect>(label, entry.payload);
    };

    if (effectId == 21u) {
      handledEffect = true;
      success = parseCommentEffect(chunkReader, payloadEnd, result);
    } else if (effectId == kListId) {
      handledEffect = true;
      std::vector<std::unique_ptr<Effect>> nestedChain;
      success = parseRenderListChunk(chunkReader, payloadEnd, result, nestedChain);
      if (success) {
        auto composite = std::make_unique<CompositeEffect>();
        for (auto& child : nestedChain) {
          composite->addEffect(std::move(child));
        }
        parsedEffect = std::move(composite);
      }
    } else {
      if (!entry.effectName.empty()) {
        const std::string key = avs::effects::legacy::normalizeLegacyToken(entry.effectName);
        auto it = registry.find(key);
        if (it != registry.end()) {
          handledEffect = true;
          parsedEffect = it->second(entry, result);
          success = static_cast<bool>(parsedEffect);
          if (!success) {
            result.warnings.push_back("failed to parse effect index: " + describeEffect(effectId));
          }
        }
      }
    }

    if (!entry.payload.empty() || effectId == 21u || effectId == kListId || !entry.effectName.empty()) {
      result.effects.push_back(entry);
    }

    if (handledEffect) {
      if (success) {
        if (parsedEffect) {
          chain.push_back(std::move(parsedEffect));
        }
      } else if (effectId != 21u) {
        chain.push_back(makeUnknown(entry.effectName));
      }
    } else {
      // Effect not handled by native registry
      // For APE effects, try Wine APE loader as fallback
      if (effectId >= kApeIdBase && !apeId.empty() && avs::ape::isWineAPESupported()) {
        std::fprintf(stderr, "INFO: Attempting to load APE plugin: %s\n", apeId.c_str());
        auto wineEffect = avs::ape::createWineAPEEffect(apeId, entry, result, result.presetPath);
        if (wineEffect) {
          std::fprintf(stderr, "INFO: Successfully loaded APE plugin via Wine emulator: %s\n", apeId.c_str());
          chain.push_back(std::move(wineEffect));
        } else {
          std::fprintf(stderr, "WARNING: Failed to load APE plugin via Wine emulator: %s\n", apeId.c_str());
          chain.push_back(makeUnknown(entry.effectName));
        }
      } else {
        // Not an APE effect, or Wine not available
        if (!entry.effectName.empty()) {
          std::string msg = "preset loader does not yet decode effect: " + describeEffect(effectId);
          if (effectId >= kApeIdBase && !apeId.empty()) {
            msg += " (APE plugin: '" + apeId + "')";
          }
          result.warnings.push_back(msg);
        } else {
          result.warnings.push_back("unsupported effect index: " + describeEffect(effectId));
        }
        chain.push_back(makeUnknown(entry.effectName));
      }
    }

    r.pos = payloadEnd;
  }

  if (r.pos < chunkEnd) {
    r.pos = chunkEnd;
  }
  return true;
}

bool parseBinaryMagicHeader(const std::vector<char>& data, size_t& headerLen, std::string& version) {
  if (data.size() <= kMagicPrefix.size()) return false;
  if (!std::equal(kMagicPrefix.begin(), kMagicPrefix.end(), data.begin())) return false;
  auto versionBegin = data.begin();
  std::advance(versionBegin, static_cast<std::vector<char>::difference_type>(kMagicPrefix.size()));
  auto terminator = std::find(versionBegin, data.end(), kMagicTerminator);
  if (terminator == data.end()) return false;
  version.assign(versionBegin, terminator);
  headerLen = static_cast<size_t>(std::distance(data.begin(), terminator)) + 1;
  return true;
}

ParsedPreset parseBinaryPreset(const std::vector<char>& data, size_t headerLen) {
  ParsedPreset result;
  Reader r{data, headerLen};
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
      std::string rest;
      std::getline(iss, rest);
      rest = trim(rest);
      std::string initScript;
      std::string frameScript;
      std::string beatScript;
      std::string pixelScript;
      ScriptedEffect::Mode mode = ScriptedEffect::Mode::kSuperscope;
      bool recompute = false;
      if (rest.find('=') == std::string::npos) {
        pixelScript = rest;
      } else {
        size_t pos = 0;
        while (pos < rest.size()) {
          while (pos < rest.size() &&
                 std::isspace(static_cast<unsigned char>(rest[pos]))) {
            ++pos;
          }
          if (pos >= rest.size()) break;
          size_t eq = rest.find('=', pos);
          if (eq == std::string::npos) {
            result.unknown.push_back(rest.substr(pos));
            break;
          }
          std::string key = trim(rest.substr(pos, eq - pos));
          pos = eq + 1;
          if (pos >= rest.size()) break;
          std::string value;
          if (rest[pos] == '"') {
            ++pos;
            size_t end = rest.find('"', pos);
            if (end == std::string::npos) {
              value = rest.substr(pos);
              pos = rest.size();
            } else {
              value = rest.substr(pos, end - pos);
              pos = end + 1;
            }
          } else {
            size_t end = rest.find(' ', pos);
            if (end == std::string::npos) {
              value = rest.substr(pos);
              pos = rest.size();
            } else {
              value = rest.substr(pos, end - pos);
              pos = end + 1;
            }
          }
          std::string keyLower;
          keyLower.resize(key.size());
          std::transform(key.begin(), key.end(), keyLower.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
          });
          if (keyLower == "init") {
            initScript = value;
          } else if (keyLower == "frame") {
            frameScript = value;
          } else if (keyLower == "beat") {
            beatScript = value;
          } else if (keyLower == "pixel" || keyLower == "point") {
            pixelScript = value;
          } else if (keyLower == "mode") {
            std::string valueLower = value;
            std::transform(valueLower.begin(), valueLower.end(), valueLower.begin(), [](unsigned char c) {
              return static_cast<char>(std::tolower(c));
            });
            if (valueLower == "color_mod" || valueLower == "colormod") {
              mode = ScriptedEffect::Mode::kColorModifier;
            } else if (valueLower == "superscope") {
              mode = ScriptedEffect::Mode::kSuperscope;
            } else {
              result.unknown.push_back("scripted:mode=" + value);
            }
          } else if (keyLower == "recompute") {
            std::string valueLower = value;
            std::transform(valueLower.begin(), valueLower.end(), valueLower.begin(), [](unsigned char c) {
              return static_cast<char>(std::tolower(c));
            });
            recompute = (valueLower == "1" || valueLower == "true");
          } else {
            result.unknown.push_back("scripted:" + key);
          }
        }
      }
      result.chain.push_back(
          std::make_unique<ScriptedEffect>(initScript, frameScript, beatScript, pixelScript, mode, recompute));
    } else {
      result.warnings.push_back("unsupported effect: " + type);
      std::fprintf(stderr, "WARNING: Unsupported text effect: %s\n", type.c_str());
      std::vector<std::uint8_t> payload(t.begin(), t.end());
      result.chain.push_back(
          std::make_unique<UnknownRenderObjectEffect>(type, std::move(payload)));
      result.unknown.push_back(t);
    }
  }
  return result;
}

}  // namespace

ParsedPreset parsePreset(const std::filesystem::path& file) {
  ParsedPreset result;
  result.presetPath = file;  // Store preset path for APE DLL discovery
  std::ifstream f(file, std::ios::binary);
  if (!f) {
    result.warnings.push_back("failed to open preset");
    return result;
  }
  std::vector<char> buffer((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  size_t headerLen = 0;
  std::string version;
  if (parseBinaryMagicHeader(buffer, headerLen, version)) {
    auto preset = parseBinaryPreset(buffer, headerLen);
    preset.presetPath = file;  // Ensure preset path is set
    if (!isKnownMagicVersion(version)) {
      preset.warnings.push_back("unknown preset version: " + version);
    }
    return preset;
  }
  auto preset = parseTextPreset(std::string(buffer.begin(), buffer.end()));
  preset.presetPath = file;  // Ensure preset path is set
  return preset;
}

}  // namespace avs
