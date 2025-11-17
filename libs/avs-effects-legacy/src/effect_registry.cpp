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

//=============================================================================
// Trans / Fadeout (ID 3)
//=============================================================================
std::unique_ptr<avs::Effect> makeFadeout(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  PayloadReader reader(entry.payload);

  std::int32_t fadelen = 0;
  std::int32_t color = 0;

  if (!reader.readI32(fadelen)) {
    preset.warnings.push_back("fadeout: truncated payload");
    return nullptr;
  }
  reader.readI32(color);  // Optional

  return std::make_unique<avs::UnknownRenderObjectEffect>("Trans / Fadeout", entry.payload);
}

//=============================================================================
// Trans / Colorfade (ID 11)
//=============================================================================
std::unique_ptr<avs::Effect> makeColorfade(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  PayloadReader reader(entry.payload);

  std::int32_t enabled = 1;
  std::int32_t faders[3] = {0, 0, 0};
  std::int32_t beatfaders[3] = {0, 0, 0};

  if (!reader.readI32(enabled)) {
    preset.warnings.push_back("colorfade: truncated payload");
    return nullptr;
  }
  reader.readI32(faders[0]);
  reader.readI32(faders[1]);
  reader.readI32(faders[2]);
  reader.readI32(beatfaders[0]);
  reader.readI32(beatfaders[1]);
  reader.readI32(beatfaders[2]);

  return std::make_unique<avs::UnknownRenderObjectEffect>("Trans / Colorfade", entry.payload);
}

//=============================================================================
// Trans / Water (ID 20)
//=============================================================================
std::unique_ptr<avs::Effect> makeWater(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  PayloadReader reader(entry.payload);

  std::int32_t enabled = 1;

  if (!reader.readI32(enabled)) {
    preset.warnings.push_back("water: truncated payload");
    return nullptr;
  }

  return std::make_unique<avs::UnknownRenderObjectEffect>("Trans / Water", entry.payload);
}

//=============================================================================
// Trans / Grain (ID 24)
//=============================================================================
std::unique_ptr<avs::Effect> makeGrain(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  PayloadReader reader(entry.payload);

  std::int32_t enabled = 1;
  std::int32_t blend = 0;
  std::int32_t blendavg = 0;
  std::int32_t smax = 0;
  std::int32_t staticgrain = 0;

  if (!reader.readI32(enabled)) {
    preset.warnings.push_back("grain: truncated payload");
    return nullptr;
  }
  reader.readI32(blend);
  reader.readI32(blendavg);
  reader.readI32(smax);
  reader.readI32(staticgrain);

  return std::make_unique<avs::UnknownRenderObjectEffect>("Trans / Grain", entry.payload);
}

//=============================================================================
// Trans / Mirror (ID 26)
//=============================================================================
std::unique_ptr<avs::Effect> makeMirror(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  PayloadReader reader(entry.payload);

  std::int32_t enabled = 1;
  std::int32_t mode = 0;
  std::int32_t onbeat = 0;
  std::int32_t smooth = 0;
  std::int32_t slower = 4;

  if (!reader.readI32(enabled)) {
    preset.warnings.push_back("mirror: truncated payload");
    return nullptr;
  }
  reader.readI32(mode);
  reader.readI32(onbeat);
  reader.readI32(smooth);
  reader.readI32(slower);

  return std::make_unique<avs::UnknownRenderObjectEffect>("Trans / Mirror", entry.payload);
}

//=============================================================================
// Trans / Bump (ID 29)
//=============================================================================
std::unique_ptr<avs::Effect> makeBump(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  PayloadReader reader(entry.payload);

  std::int32_t enabled = 1;
  std::int32_t onbeat = 0;
  std::int32_t durFrames = 15;
  std::int32_t depth = 1;
  std::int32_t depth2 = 1;
  std::int32_t blend = 0;

  if (!reader.readI32(enabled)) {
    preset.warnings.push_back("bump: truncated payload");
    return nullptr;
  }
  reader.readI32(onbeat);
  reader.readI32(durFrames);
  reader.readI32(depth);
  reader.readI32(depth2);
  reader.readI32(blend);

  return std::make_unique<avs::UnknownRenderObjectEffect>("Trans / Bump", entry.payload);
}

//=============================================================================
// Trans / Invert (ID 37)
//=============================================================================
std::unique_ptr<avs::Effect> makeInvert(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  PayloadReader reader(entry.payload);

  std::int32_t enabled = 1;

  if (!reader.readI32(enabled)) {
    preset.warnings.push_back("invert: truncated payload");
    return nullptr;
  }

  return std::make_unique<avs::UnknownRenderObjectEffect>("Trans / Invert", entry.payload);
}

//=============================================================================
// Render / OnBeat Clear (ID 5)
//=============================================================================
std::unique_ptr<avs::Effect> makeOnBeatClear(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  (void)preset;
  PayloadReader reader(entry.payload);

  std::int32_t color = 0;
  std::int32_t blend = 0;
  std::int32_t nf = 0;

  reader.readI32(color);
  reader.readI32(blend);
  reader.readI32(nf);

  return std::make_unique<avs::UnknownRenderObjectEffect>("Render / OnBeat Clear", entry.payload);
}

//=============================================================================
// Trans / Color Clip (ID 12)
//=============================================================================
std::unique_ptr<avs::Effect> makeColorClip(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  (void)preset;
  PayloadReader reader(entry.payload);

  std::int32_t enabled = 1;
  std::int32_t color_clip = 0;
  std::int32_t color_clip_out = 0;
  std::int32_t color_dist = 0;

  reader.readI32(enabled);
  reader.readI32(color_clip);
  reader.readI32(color_clip_out);
  reader.readI32(color_dist);

  return std::make_unique<avs::UnknownRenderObjectEffect>("Trans / Color Clip", entry.payload);
}

//=============================================================================
// Render / Dot Grid (ID 17)
//=============================================================================
std::unique_ptr<avs::Effect> makeDotGrid(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  PayloadReader reader(entry.payload);

  std::int32_t num_colors = 0;

  if (!reader.readI32(num_colors)) {
    return std::make_unique<avs::UnknownRenderObjectEffect>("Render / Dot Grid", entry.payload);
  }

  // Validate and read color array (max 16 colors)
  if (num_colors < 0 || num_colors > 16) {
    preset.warnings.push_back("dot grid: invalid num_colors");
    return nullptr;
  }

  for (int i = 0; i < num_colors; ++i) {
    std::int32_t color = 0;
    reader.readI32(color);
  }

  std::int32_t spacing = 0, x_move = 0, y_move = 0, blend = 0;
  reader.readI32(spacing);
  reader.readI32(x_move);
  reader.readI32(y_move);
  reader.readI32(blend);

  return std::make_unique<avs::UnknownRenderObjectEffect>("Render / Dot Grid", entry.payload);
}

//=============================================================================
// Render / Dot Fountain (ID 19)
//=============================================================================
std::unique_ptr<avs::Effect> makeDotFountain(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  (void)preset;
  PayloadReader reader(entry.payload);

  std::int32_t rotvel = 0;
  std::int32_t colors[5] = {0, 0, 0, 0, 0};
  std::int32_t angle = 0;
  std::int32_t r = 0;

  reader.readI32(rotvel);
  for (int i = 0; i < 5; ++i) {
    reader.readI32(colors[i]);
  }
  reader.readI32(angle);
  reader.readI32(r);

  return std::make_unique<avs::UnknownRenderObjectEffect>("Render / Dot Fountain", entry.payload);
}

//=============================================================================
// Trans / Interleave (ID 23)
//=============================================================================
std::unique_ptr<avs::Effect> makeInterleave(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  (void)preset;
  PayloadReader reader(entry.payload);

  std::int32_t enabled = 1;
  std::int32_t x = 1, y = 1;
  std::int32_t color = 0;
  std::int32_t blend = 0, blendavg = 0;
  std::int32_t onbeat = 0, x2 = 1, y2 = 1, beatdur = 4;

  reader.readI32(enabled);
  reader.readI32(x);
  reader.readI32(y);
  reader.readI32(color);
  reader.readI32(blend);
  reader.readI32(blendavg);
  reader.readI32(onbeat);
  reader.readI32(x2);
  reader.readI32(y2);
  reader.readI32(beatdur);

  return std::make_unique<avs::UnknownRenderObjectEffect>("Trans / Interleave", entry.payload);
}

}  // namespace

AVS_LEGACY_REGISTER_EFFECT("Render / Simple", makeSimple);
AVS_LEGACY_REGISTER_EFFECT("Trans / Movement", makeMovement);
AVS_LEGACY_REGISTER_EFFECT("Trans / Fadeout", makeFadeout);
AVS_LEGACY_REGISTER_EFFECT("Render / OnBeat Clear", makeOnBeatClear);
AVS_LEGACY_REGISTER_EFFECT("Trans / Blur", makeBlur);
AVS_LEGACY_REGISTER_EFFECT("Trans / Colorfade", makeColorfade);
AVS_LEGACY_REGISTER_EFFECT("Trans / Color Clip", makeColorClip);
AVS_LEGACY_REGISTER_EFFECT("Trans / Scatter", makeScatter);
AVS_LEGACY_REGISTER_EFFECT("Render / Dot Grid", makeDotGrid);
AVS_LEGACY_REGISTER_EFFECT("Render / Dot Fountain", makeDotFountain);
AVS_LEGACY_REGISTER_EFFECT("Trans / Water", makeWater);
AVS_LEGACY_REGISTER_EFFECT("Trans / Interleave", makeInterleave);
AVS_LEGACY_REGISTER_EFFECT("Trans / Grain", makeGrain);
AVS_LEGACY_REGISTER_EFFECT("Trans / Mirror", makeMirror);
AVS_LEGACY_REGISTER_EFFECT("Trans / Bump", makeBump);
AVS_LEGACY_REGISTER_EFFECT("Trans / Mosaic", makeMosaic);
AVS_LEGACY_REGISTER_EFFECT("Trans / Brightness", makeBrightness);
AVS_LEGACY_REGISTER_EFFECT("Trans / Invert", makeInvert);
AVS_LEGACY_REGISTER_EFFECT("Trans / Color Modifier", makeColorModifier);

}  // namespace avs::effects::legacy
