#include <avs/effects/effect_registry.hpp>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace avs::effects::legacy {
namespace {

struct PayloadReader {
  explicit PayloadReader(const std::vector<std::uint8_t>& bytes) : data(bytes) {}

  bool readByte(std::uint8_t& value) {
    if (pos >= data.size()) return false;
    value = data[pos++];
    return true;
  }

  bool readU32(std::uint32_t& value) {
    if (pos + 4 > data.size()) return false;
    value = static_cast<std::uint32_t>(data[pos]) | (static_cast<std::uint32_t>(data[pos + 1]) << 8) |
            (static_cast<std::uint32_t>(data[pos + 2]) << 16) |
            (static_cast<std::uint32_t>(data[pos + 3]) << 24);
    pos += 4;
    return true;
  }

  bool readBytes(std::size_t count, std::string& out) {
    if (pos + count > data.size()) return false;
    out.assign(reinterpret_cast<const char*>(data.data() + pos), count);
    pos += count;
    return true;
  }

  const std::vector<std::uint8_t>& data;
  std::size_t pos = 0;
};

std::unique_ptr<avs::Effect> makeColorModifier(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  if (entry.payload.empty()) {
    preset.warnings.push_back("color modifier payload empty");
    return nullptr;
  }

  PayloadReader reader(entry.payload);
  std::uint8_t version = 0;
  if (!reader.readByte(version) || version != 1u) {
    preset.warnings.push_back("color modifier has unsupported version");
    return nullptr;
  }

  std::array<std::string, 4> scripts;
  for (std::size_t i = 0; i < scripts.size(); ++i) {
    std::uint32_t length = 0;
    if (!reader.readU32(length)) {
      preset.warnings.push_back("color modifier script length truncated");
      return nullptr;
    }
    if (!reader.readBytes(length, scripts[i])) {
      preset.warnings.push_back("color modifier script payload truncated");
      return nullptr;
    }
    if (!scripts[i].empty() && scripts[i].back() == '\0') {
      scripts[i].pop_back();
    }
  }

  std::uint32_t recompute = 0;
  if (!reader.readU32(recompute)) {
    preset.warnings.push_back("color modifier missing recompute flag");
    return nullptr;
  }

  return std::make_unique<avs::ScriptedEffect>(scripts[3],
                                               scripts[1],
                                               scripts[2],
                                               scripts[0],
                                               avs::ScriptedEffect::Mode::kColorModifier,
                                               recompute != 0u);
}

}  // namespace

AVS_LEGACY_REGISTER_EFFECT("Trans / Color Modifier", makeColorModifier);

}  // namespace avs::effects::legacy

