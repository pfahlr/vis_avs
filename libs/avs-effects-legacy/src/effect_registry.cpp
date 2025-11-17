#include <avs/effects/effect_registry.hpp>

#include <avs/core/ParamBlock.hpp>

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

  bool readI32(std::int32_t& value) {
    std::uint32_t raw = 0;
    if (!readU32(raw)) return false;
    value = static_cast<std::int32_t>(raw);
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

//=============================================================================
// Trans / Blur (ID 6)
//=============================================================================
std::unique_ptr<avs::Effect> makeBlur(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  PayloadReader reader(entry.payload);

  std::int32_t enabled = 1;
  std::int32_t roundmode = 0;

  if (!reader.readI32(enabled)) {
    preset.warnings.push_back("blur: truncated payload");
    return nullptr;
  }
  reader.readI32(roundmode);  // Optional field

  // Use UnknownRenderObjectEffect to preserve binary data
  return std::make_unique<avs::UnknownRenderObjectEffect>("Trans / Blur", entry.payload);
}

//=============================================================================
// Trans / Scatter (ID 16)
//=============================================================================
std::unique_ptr<avs::Effect> makeScatter(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  PayloadReader reader(entry.payload);

  std::int32_t enabled = 1;

  if (!reader.readI32(enabled)) {
    preset.warnings.push_back("scatter: truncated payload");
    return nullptr;
  }

  // Use UnknownRenderObjectEffect to preserve binary data
  return std::make_unique<avs::UnknownRenderObjectEffect>("Trans / Scatter", entry.payload);
}

//=============================================================================
// Trans / Mosaic (ID 30)
//=============================================================================
std::unique_ptr<avs::Effect> makeMosaic(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  PayloadReader reader(entry.payload);

  std::int32_t enabled = 1;
  std::int32_t quality = 50;
  std::int32_t quality2 = 50;
  std::int32_t blend = 0;
  std::int32_t blendavg = 0;
  std::int32_t onbeat = 0;
  std::int32_t durFrames = 15;

  if (!reader.readI32(enabled)) {
    preset.warnings.push_back("mosaic: truncated payload");
    return nullptr;
  }
  reader.readI32(quality);
  reader.readI32(quality2);
  reader.readI32(blend);
  reader.readI32(blendavg);
  reader.readI32(onbeat);
  reader.readI32(durFrames);

  // Use UnknownRenderObjectEffect to preserve binary data
  return std::make_unique<avs::UnknownRenderObjectEffect>("Trans / Mosaic", entry.payload);
}

//=============================================================================
// Trans / Brightness (ID 22)
//=============================================================================
std::unique_ptr<avs::Effect> makeBrightness(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  PayloadReader reader(entry.payload);

  std::int32_t enabled = 1;
  std::int32_t blend = 0;
  std::int32_t blendavg = 0;
  std::int32_t redp = 0;
  std::int32_t greenp = 0;
  std::int32_t bluep = 0;
  std::int32_t dissoc = 0;
  std::int32_t color = 0xFFFFFF;
  std::int32_t exclude = 0;
  std::int32_t distance = 16;

  if (!reader.readI32(enabled)) {
    preset.warnings.push_back("brightness: truncated payload");
    return nullptr;
  }
  reader.readI32(blend);
  reader.readI32(blendavg);
  reader.readI32(redp);
  reader.readI32(greenp);
  reader.readI32(bluep);
  reader.readI32(dissoc);
  reader.readI32(color);
  reader.readI32(exclude);
  reader.readI32(distance);

  // Use UnknownRenderObjectEffect to preserve binary data
  return std::make_unique<avs::UnknownRenderObjectEffect>("Trans / Brightness", entry.payload);
}

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

//=============================================================================
// Render / Simple (ID 0)
//=============================================================================
std::unique_ptr<avs::Effect> makeSimple(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  PayloadReader reader(entry.payload);

  std::int32_t effect = 0;
  std::int32_t num_colors = 0;

  if (!reader.readI32(effect)) {
    preset.warnings.push_back("simple: truncated payload");
    return nullptr;
  }
  if (!reader.readI32(num_colors)) {
    preset.warnings.push_back("simple: missing num_colors");
    return nullptr;
  }

  // Validate num_colors and read color array
  if (num_colors < 0 || num_colors > 16) {
    preset.warnings.push_back("simple: invalid num_colors");
    return nullptr;
  }

  for (int i = 0; i < num_colors; ++i) {
    std::int32_t color = 0;
    if (!reader.readI32(color)) {
      preset.warnings.push_back("simple: truncated color array");
      return nullptr;
    }
  }

  // Use UnknownRenderObjectEffect to preserve binary data
  return std::make_unique<avs::UnknownRenderObjectEffect>("Render / Simple", entry.payload);
}

//=============================================================================
// Trans / Movement (ID 15)
//=============================================================================
std::unique_ptr<avs::Effect> makeMovement(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  PayloadReader reader(entry.payload);

  std::int32_t effect = 0;

  if (!reader.readI32(effect)) {
    preset.warnings.push_back("movement: truncated payload");
    return nullptr;
  }

  // Movement has complex variable-length format depending on effect value
  // For now, just validate the first field and preserve the data

  // Use UnknownRenderObjectEffect to preserve binary data
  return std::make_unique<avs::UnknownRenderObjectEffect>("Trans / Movement", entry.payload);
}

}  // namespace

AVS_LEGACY_REGISTER_EFFECT("Render / Simple", makeSimple);
AVS_LEGACY_REGISTER_EFFECT("Trans / Movement", makeMovement);
AVS_LEGACY_REGISTER_EFFECT("Trans / Blur", makeBlur);
AVS_LEGACY_REGISTER_EFFECT("Trans / Scatter", makeScatter);
AVS_LEGACY_REGISTER_EFFECT("Trans / Mosaic", makeMosaic);
AVS_LEGACY_REGISTER_EFFECT("Trans / Brightness", makeBrightness);
AVS_LEGACY_REGISTER_EFFECT("Trans / Color Modifier", makeColorModifier);

}  // namespace avs::effects::legacy
