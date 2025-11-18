#include <avs/effects/effect_registry.hpp>

#include <avs/core/ParamBlock.hpp>
#include <avs/effects/trans/effect_add_borders.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

// Forward declarations for effect factory functions
namespace avs {
std::unique_ptr<Effect> createMovementEffect(int effect, bool blend, bool sourcemapped,
                                              bool rectangular, bool subpixel, bool wrap,
                                              const std::string& effect_exp);
}

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

  bool readFloat(float& value) {
    std::uint32_t raw = 0;
    if (!readU32(raw)) return false;
    std::memcpy(&value, &raw, sizeof(float));
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
  std::int32_t blend = 0;
  std::int32_t sourcemapped = 0;
  std::int32_t rectangular = 0;
  std::int32_t subpixel = 1;  // Default to enabled
  std::int32_t wrap = 0;
  std::string effect_exp;

  if (!reader.readI32(effect)) {
    preset.warnings.push_back("movement: truncated payload");
    return nullptr;
  }

  // If effect is 32767, there's an EEL expression following
  if (effect == 32767) {
    // Check for "!rect " prefix
    if (reader.pos + 6 <= entry.payload.size()) {
      if (std::memcmp(entry.payload.data() + reader.pos, "!rect ", 6) == 0) {
        reader.pos += 6;
        rectangular = 1;
      }
    }

    // Read version byte
    std::uint8_t version = 0;
    if (reader.pos < entry.payload.size()) {
      version = entry.payload[reader.pos++];
    }

    if (version == 1) {
      // New format: 4-byte length + string
      std::uint32_t length = 0;
      if (!reader.readU32(length)) {
        preset.warnings.push_back("movement: truncated expression length");
        return nullptr;
      }
      if (length > 0 && reader.pos + length <= entry.payload.size()) {
        effect_exp.assign(reinterpret_cast<const char*>(entry.payload.data() + reader.pos), length);
        reader.pos += length;
        // Remove null terminator if present
        if (!effect_exp.empty() && effect_exp.back() == '\0') {
          effect_exp.pop_back();
        }
      }
    } else if (reader.pos + 256 <= entry.payload.size()) {
      // Old format: 256-byte fixed string (or less if rectangular prefix was found)
      size_t str_len = 256 - (rectangular ? 6 : 0);
      if (reader.pos + str_len <= entry.payload.size()) {
        effect_exp.assign(reinterpret_cast<const char*>(entry.payload.data() + reader.pos), str_len);
        // Trim at null terminator
        size_t null_pos = effect_exp.find('\0');
        if (null_pos != std::string::npos) {
          effect_exp.resize(null_pos);
        }
        reader.pos += str_len;
      }
    }
  }

  // Read additional parameters
  reader.readI32(blend);
  reader.readI32(sourcemapped);
  reader.readI32(rectangular);
  reader.readI32(subpixel);
  reader.readI32(wrap);

  // Check for effect ID at end (for effects > 15)
  if (effect == 0 && reader.pos + 4 <= entry.payload.size()) {
    reader.readI32(effect);
  }

  // Validate effect range
  if (effect != 32767 && (effect > 23 || effect < 0)) {
    effect = 0;
  }

  return avs::createMovementEffect(effect, blend != 0, sourcemapped != 0, rectangular != 0,
                                    subpixel != 0, wrap != 0, effect_exp);
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

//=============================================================================
// Trans / Blitter Feedback (ID 4)
//=============================================================================
std::unique_ptr<avs::Effect> makeBlitterFeedback(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  (void)preset;
  PayloadReader reader(entry.payload);

  std::int32_t scale = 0, scale2 = 0, blend = 0, beatch = 0, subpixel = 0;

  reader.readI32(scale);
  reader.readI32(scale2);
  reader.readI32(blend);
  reader.readI32(beatch);
  reader.readI32(subpixel);

  return std::make_unique<avs::UnknownRenderObjectEffect>("Trans / Blitter Feedback", entry.payload);
}

//=============================================================================
// Trans / Roto Blitter (ID 9)
//=============================================================================
std::unique_ptr<avs::Effect> makeRotoBlitter(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  (void)preset;
  PayloadReader reader(entry.payload);

  std::int32_t zoom_scale = 0, rot_dir = 0, blend = 0, beatch = 0;
  std::int32_t beatch_speed = 0, zoom_scale2 = 0, beatch_scale = 0, subpixel = 0;

  reader.readI32(zoom_scale);
  reader.readI32(rot_dir);
  reader.readI32(blend);
  reader.readI32(beatch);
  reader.readI32(beatch_speed);
  reader.readI32(zoom_scale2);
  reader.readI32(beatch_scale);
  reader.readI32(subpixel);

  return std::make_unique<avs::UnknownRenderObjectEffect>("Trans / Roto Blitter", entry.payload);
}

//=============================================================================
// Render / Clear screen (ID 25)
//=============================================================================
std::unique_ptr<avs::Effect> makeClearScreen(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  (void)preset;
  PayloadReader reader(entry.payload);

  std::int32_t enabled = 1, color = 0, blend = 0, blendavg = 0, onlyfirst = 0;

  reader.readI32(enabled);
  reader.readI32(color);
  reader.readI32(blend);
  reader.readI32(blendavg);
  reader.readI32(onlyfirst);

  return std::make_unique<avs::UnknownRenderObjectEffect>("Render / Clear screen", entry.payload);
}

//=============================================================================
// Render / Starfield (ID 27)
//=============================================================================
std::unique_ptr<avs::Effect> makeStarfield(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  (void)preset;
  PayloadReader reader(entry.payload);

  std::int32_t enabled = 1, color = 0, blend = 0, blendavg = 0, MaxStars_set = 0;
  std::int32_t onbeat = 0, durFrames = 0;
  float WarpSpeed = 0.0f, spdBeat = 0.0f;

  reader.readI32(enabled);
  reader.readI32(color);
  reader.readI32(blend);
  reader.readI32(blendavg);
  reader.readFloat(WarpSpeed);
  reader.readI32(MaxStars_set);
  reader.readI32(onbeat);
  reader.readFloat(spdBeat);
  reader.readI32(durFrames);

  return std::make_unique<avs::UnknownRenderObjectEffect>("Render / Starfield", entry.payload);
}

//=============================================================================
// Trans / Water Bump (ID 31)
//=============================================================================
std::unique_ptr<avs::Effect> makeWaterBump(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  (void)preset;
  PayloadReader reader(entry.payload);

  std::int32_t enabled = 1, density = 0, depth = 0, random_drop = 0;
  std::int32_t drop_position_x = 0, drop_position_y = 0, drop_radius = 0, method = 0;

  reader.readI32(enabled);
  reader.readI32(density);
  reader.readI32(depth);
  reader.readI32(random_drop);
  reader.readI32(drop_position_x);
  reader.readI32(drop_position_y);
  reader.readI32(drop_radius);
  reader.readI32(method);

  return std::make_unique<avs::UnknownRenderObjectEffect>("Trans / Water Bump", entry.payload);
}

//=============================================================================
// Trans / Unique tone (ID 38)
//=============================================================================
std::unique_ptr<avs::Effect> makeUniqueTone(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  (void)preset;
  PayloadReader reader(entry.payload);

  std::int32_t enabled = 1, color = 0, blend = 0, blendavg = 0, invert = 0;

  reader.readI32(enabled);
  reader.readI32(color);
  reader.readI32(blend);
  reader.readI32(blendavg);
  reader.readI32(invert);

  return std::make_unique<avs::UnknownRenderObjectEffect>("Trans / Unique tone", entry.payload);
}

//=============================================================================
// Trans / Interferences (ID 41)
//=============================================================================
std::unique_ptr<avs::Effect> makeInterferences(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  (void)preset;
  PayloadReader reader(entry.payload);

  std::int32_t enabled = 1, nPoints = 2, rotation = 0, distance = 20, alpha = 128;
  std::int32_t rotationinc = 0, blend = 0, blendavg = 0;
  std::int32_t distance2 = 20, alpha2 = 128, rotationinc2 = 0, rgb = 0, onbeat = 0;
  float speed = 0.0f;

  reader.readI32(enabled);
  reader.readI32(nPoints);
  reader.readI32(rotation);
  reader.readI32(distance);
  reader.readI32(alpha);
  reader.readI32(rotationinc);
  reader.readI32(blend);
  reader.readI32(blendavg);
  reader.readI32(distance2);
  reader.readI32(alpha2);
  reader.readI32(rotationinc2);
  reader.readI32(rgb);
  reader.readI32(onbeat);
  reader.readFloat(speed);

  return std::make_unique<avs::UnknownRenderObjectEffect>("Trans / Interferences", entry.payload);
}

//=============================================================================
// Trans / Dynamic Shift (ID 42)
//=============================================================================
std::unique_ptr<avs::Effect> makeDynamicShift(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  (void)preset;
  // Complex variable-length format with EEL strings - just preserve binary data
  return std::make_unique<avs::UnknownRenderObjectEffect>("Trans / Dynamic Shift", entry.payload);
}

//=============================================================================
// Trans / Dynamic Movement (ID 43)
//=============================================================================
std::unique_ptr<avs::Effect> makeDynamicMovement(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  (void)preset;
  // Complex variable-length format with EEL strings - just preserve binary data
  return std::make_unique<avs::UnknownRenderObjectEffect>("Trans / Dynamic Movement", entry.payload);
}

//=============================================================================
// Trans / Fast Brightness (ID 44)
//=============================================================================
std::unique_ptr<avs::Effect> makeFastBrightness(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  (void)preset;
  PayloadReader reader(entry.payload);

  std::int32_t dir = 0;

  reader.readI32(dir);

  return std::make_unique<avs::UnknownRenderObjectEffect>("Trans / Fast Brightness", entry.payload);
}

//=============================================================================
// Render / Dot Plane (ID 1)
//=============================================================================
std::unique_ptr<avs::Effect> makeDotPlane(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  (void)preset;
  PayloadReader reader(entry.payload);

  std::int32_t rotvel = 0;
  std::int32_t colors[5] = {0, 0, 0, 0, 0};
  std::int32_t angle = 0, r_scaled_int = 0;

  reader.readI32(rotvel);
  for (int i = 0; i < 5; ++i) reader.readI32(colors[i]);
  reader.readI32(angle);
  reader.readI32(r_scaled_int);

  return std::make_unique<avs::UnknownRenderObjectEffect>("Render / Dot Plane", entry.payload);
}

//=============================================================================
// Render / Oscilliscope Star (ID 2)
//=============================================================================
std::unique_ptr<avs::Effect> makeOscilloscopeStar(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  // Variable-length format - preserve binary data
  (void)preset;
  return std::make_unique<avs::UnknownRenderObjectEffect>("Render / Oscilliscope Star", entry.payload);
}

//=============================================================================
// Render / Bass Spin (ID 7)
//=============================================================================
std::unique_ptr<avs::Effect> makeBassSpin(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  (void)preset;
  PayloadReader reader(entry.payload);

  std::int32_t enabled = 1, colors[2] = {0, 0}, mode = 0;

  reader.readI32(enabled);
  reader.readI32(colors[0]);
  reader.readI32(colors[1]);
  reader.readI32(mode);

  return std::make_unique<avs::UnknownRenderObjectEffect>("Render / Bass Spin", entry.payload);
}

//=============================================================================
// Render / Moving Particle (ID 8)
//=============================================================================
std::unique_ptr<avs::Effect> makeMovingParticle(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  (void)preset;
  PayloadReader reader(entry.payload);

  std::int32_t enabled = 1, colors = 0, maxdist = 0, size = 0, size2 = 0, blend = 0;

  reader.readI32(enabled);
  reader.readI32(colors);
  reader.readI32(maxdist);
  reader.readI32(size);
  reader.readI32(size2);
  reader.readI32(blend);

  return std::make_unique<avs::UnknownRenderObjectEffect>("Render / Moving Particle", entry.payload);
}

//=============================================================================
// Render / SVP Loader (ID 10)
//=============================================================================
std::unique_ptr<avs::Effect> makeSVPLoader(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  // Variable-length string format - preserve binary data
  (void)preset;
  return std::make_unique<avs::UnknownRenderObjectEffect>("Render / SVP Loader", entry.payload);
}

//=============================================================================
// Render / Rotating Stars (ID 13)
//=============================================================================
std::unique_ptr<avs::Effect> makeRotatingStars(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  // Variable-length format - preserve binary data
  (void)preset;
  return std::make_unique<avs::UnknownRenderObjectEffect>("Render / Rotating Stars", entry.payload);
}

//=============================================================================
// Render / Ring (ID 14)
//=============================================================================
std::unique_ptr<avs::Effect> makeRing(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  // Variable-length format - preserve binary data
  (void)preset;
  return std::make_unique<avs::UnknownRenderObjectEffect>("Render / Ring", entry.payload);
}

//=============================================================================
// Misc / Buffer Save (ID 18)
//=============================================================================
std::unique_ptr<avs::Effect> makeBufferSave(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  (void)preset;
  PayloadReader reader(entry.payload);

  std::int32_t dir = 0, which = 0, blend = 0, adjblend_val = 0;

  reader.readI32(dir);
  reader.readI32(which);
  reader.readI32(blend);
  reader.readI32(adjblend_val);

  return std::make_unique<avs::UnknownRenderObjectEffect>("Misc / Buffer Save", entry.payload);
}

//=============================================================================
// Render / Text (ID 28)
//=============================================================================
std::unique_ptr<avs::Effect> makeText(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  // Complex variable-length format with structs and strings - preserve binary data
  (void)preset;
  return std::make_unique<avs::UnknownRenderObjectEffect>("Render / Text", entry.payload);
}

//=============================================================================
// Render / AVI (ID 32)
//=============================================================================
std::unique_ptr<avs::Effect> makeAVI(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  // Variable-length filename - preserve binary data
  (void)preset;
  return std::make_unique<avs::UnknownRenderObjectEffect>("Render / AVI", entry.payload);
}

//=============================================================================
// Misc / Custom BPM (ID 33)
//=============================================================================
std::unique_ptr<avs::Effect> makeCustomBPM(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  (void)preset;
  PayloadReader reader(entry.payload);

  std::int32_t enabled = 1, arbitrary = 0, skip = 0, invert = 0, arbVal = 60, skipVal = 2, skipfirst = 0;

  reader.readI32(enabled);
  reader.readI32(arbitrary);
  reader.readI32(skip);
  reader.readI32(invert);
  reader.readI32(arbVal);
  reader.readI32(skipVal);
  reader.readI32(skipfirst);

  return std::make_unique<avs::UnknownRenderObjectEffect>("Misc / Custom BPM", entry.payload);
}

//=============================================================================
// Render / Picture (ID 34)
//=============================================================================
std::unique_ptr<avs::Effect> makePicture(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  // Variable-length filename - preserve binary data
  (void)preset;
  return std::make_unique<avs::UnknownRenderObjectEffect>("Render / Picture", entry.payload);
}

//=============================================================================
// Trans / Dynamic Distance Modifier (ID 35)
//=============================================================================
std::unique_ptr<avs::Effect> makeDynamicDistanceModifier(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  // Complex variable-length EEL expressions - preserve binary data
  (void)preset;
  return std::make_unique<avs::UnknownRenderObjectEffect>("Trans / Dynamic Distance Modifier", entry.payload);
}

//=============================================================================
// Render / SuperScope (ID 36)
//=============================================================================
std::unique_ptr<avs::Effect> makeSuperScope(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  // Complex variable-length format with EEL + colors - preserve binary data
  (void)preset;
  return std::make_unique<avs::UnknownRenderObjectEffect>("Render / SuperScope", entry.payload);
}

//=============================================================================
// Render / Timescope (ID 39)
//=============================================================================
std::unique_ptr<avs::Effect> makeTimescope(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  (void)preset;
  PayloadReader reader(entry.payload);

  std::int32_t enabled = 1, color = 0, blend = 0, blendavg = 0, which_ch = 0, nbands = 0;

  reader.readI32(enabled);
  reader.readI32(color);
  reader.readI32(blend);
  reader.readI32(blendavg);
  reader.readI32(which_ch);
  reader.readI32(nbands);

  return std::make_unique<avs::UnknownRenderObjectEffect>("Render / Timescope", entry.payload);
}

//=============================================================================
// Misc / Set render mode (ID 40)
//=============================================================================
std::unique_ptr<avs::Effect> makeSetRenderMode(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  (void)preset;
  PayloadReader reader(entry.payload);

  std::int32_t newmode = 0;  // Packed bitfield

  reader.readI32(newmode);

  return std::make_unique<avs::UnknownRenderObjectEffect>("Misc / Set render mode", entry.payload);
}

}  // namespace

AVS_LEGACY_REGISTER_EFFECT("Render / Simple", makeSimple);
AVS_LEGACY_REGISTER_EFFECT("Render / Dot Plane", makeDotPlane);
AVS_LEGACY_REGISTER_EFFECT("Render / Oscilliscope Star", makeOscilloscopeStar);
AVS_LEGACY_REGISTER_EFFECT("Render / Oscilloscope Star", makeOscilloscopeStar);  // Correct spelling
AVS_LEGACY_REGISTER_EFFECT("Trans / Movement", makeMovement);
AVS_LEGACY_REGISTER_EFFECT("Trans / Fadeout", makeFadeout);
AVS_LEGACY_REGISTER_EFFECT("Trans / Blitter Feedback", makeBlitterFeedback);
AVS_LEGACY_REGISTER_EFFECT("Render / OnBeat Clear", makeOnBeatClear);
AVS_LEGACY_REGISTER_EFFECT("Trans / Blur", makeBlur);
AVS_LEGACY_REGISTER_EFFECT("Render / Bass Spin", makeBassSpin);
AVS_LEGACY_REGISTER_EFFECT("Render / Moving Particle", makeMovingParticle);
AVS_LEGACY_REGISTER_EFFECT("Trans / Roto Blitter", makeRotoBlitter);
AVS_LEGACY_REGISTER_EFFECT("Render / SVP Loader", makeSVPLoader);
AVS_LEGACY_REGISTER_EFFECT("Trans / Colorfade", makeColorfade);
AVS_LEGACY_REGISTER_EFFECT("Trans / Color Clip", makeColorClip);
AVS_LEGACY_REGISTER_EFFECT("Render / Rotating Stars", makeRotatingStars);
AVS_LEGACY_REGISTER_EFFECT("Render / Ring", makeRing);
AVS_LEGACY_REGISTER_EFFECT("Trans / Scatter", makeScatter);
AVS_LEGACY_REGISTER_EFFECT("Render / Dot Grid", makeDotGrid);
AVS_LEGACY_REGISTER_EFFECT("Misc / Buffer Save", makeBufferSave);
AVS_LEGACY_REGISTER_EFFECT("Render / Dot Fountain", makeDotFountain);
AVS_LEGACY_REGISTER_EFFECT("Trans / Water", makeWater);
AVS_LEGACY_REGISTER_EFFECT("Trans / Interleave", makeInterleave);
AVS_LEGACY_REGISTER_EFFECT("Trans / Grain", makeGrain);
AVS_LEGACY_REGISTER_EFFECT("Render / Clear screen", makeClearScreen);
AVS_LEGACY_REGISTER_EFFECT("Trans / Mirror", makeMirror);
AVS_LEGACY_REGISTER_EFFECT("Render / Starfield", makeStarfield);
AVS_LEGACY_REGISTER_EFFECT("Render / Text", makeText);
AVS_LEGACY_REGISTER_EFFECT("Trans / Bump", makeBump);
AVS_LEGACY_REGISTER_EFFECT("Trans / Mosaic", makeMosaic);
AVS_LEGACY_REGISTER_EFFECT("Trans / Water Bump", makeWaterBump);
AVS_LEGACY_REGISTER_EFFECT("Render / AVI", makeAVI);
AVS_LEGACY_REGISTER_EFFECT("Misc / Custom BPM", makeCustomBPM);
AVS_LEGACY_REGISTER_EFFECT("Render / Picture", makePicture);
AVS_LEGACY_REGISTER_EFFECT("Trans / Dynamic Distance Modifier", makeDynamicDistanceModifier);
AVS_LEGACY_REGISTER_EFFECT("Render / SuperScope", makeSuperScope);
AVS_LEGACY_REGISTER_EFFECT("Trans / Brightness", makeBrightness);
AVS_LEGACY_REGISTER_EFFECT("Trans / Invert", makeInvert);
AVS_LEGACY_REGISTER_EFFECT("Trans / Unique tone", makeUniqueTone);
AVS_LEGACY_REGISTER_EFFECT("Render / Timescope", makeTimescope);
AVS_LEGACY_REGISTER_EFFECT("Misc / Set render mode", makeSetRenderMode);
AVS_LEGACY_REGISTER_EFFECT("Trans / Interferences", makeInterferences);
AVS_LEGACY_REGISTER_EFFECT("Trans / Dynamic Shift", makeDynamicShift);
AVS_LEGACY_REGISTER_EFFECT("Trans / Dynamic Movement", makeDynamicMovement);
AVS_LEGACY_REGISTER_EFFECT("Trans / Fast Brightness", makeFastBrightness);
AVS_LEGACY_REGISTER_EFFECT("Trans / Color Modifier", makeColorModifier);

//=============================================================================
// Trans / Add Borders
//=============================================================================
std::unique_ptr<avs::Effect> makeAddBorders(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  (void)preset;
  PayloadReader reader(entry.payload);

  std::int32_t enabled = 1;
  std::uint32_t color = 0x000000;
  std::int32_t size = 10;

  reader.readI32(enabled);
  reader.readU32(color);
  reader.readI32(size);

  auto effect = std::make_unique<avs::effects::trans::AddBorders>();

  avs::core::ParamBlock params;
  params.set("enabled", enabled != 0);
  params.set("color", static_cast<int>(color));
  params.set("size", size);
  effect->setParams(params);

  return effect;
}

AVS_LEGACY_REGISTER_EFFECT("Trans / Add Borders", makeAddBorders);

//=============================================================================
// Trans / Channel Shift (APE)
//=============================================================================
std::unique_ptr<avs::Effect> makeChannelShift(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  (void)preset;
  PayloadReader reader(entry.payload);

  std::int32_t mode = 0, onbeat = 0;

  reader.readI32(mode);
  reader.readI32(onbeat);

  return std::make_unique<avs::UnknownRenderObjectEffect>("Trans / Channel Shift", entry.payload);
}

//=============================================================================
// Trans / Color Reduction (APE)
//=============================================================================
std::unique_ptr<avs::Effect> makeColorReduction(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  (void)preset;
  // Format: char fname[260] + int32 levels = 264 bytes
  // Just preserve binary data as this is complex
  return std::make_unique<avs::UnknownRenderObjectEffect>("Trans / Color Reduction", entry.payload);
}

//=============================================================================
// Trans / Multiplier (APE)
//=============================================================================
std::unique_ptr<avs::Effect> makeMultiplier(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  (void)preset;
  PayloadReader reader(entry.payload);

  std::int32_t ml = 0;

  reader.readI32(ml);

  return std::make_unique<avs::UnknownRenderObjectEffect>("Trans / Multiplier", entry.payload);
}

//=============================================================================
// Trans / Multi Delay (APE)
//=============================================================================
std::unique_ptr<avs::Effect> makeMultiDelay(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  (void)preset;
  PayloadReader reader(entry.payload);

  std::int32_t mode = 0, activebuffer = 0;
  std::int32_t usebeats[6] = {0, 0, 0, 0, 0, 0};
  std::int32_t delay[6] = {0, 0, 0, 0, 0, 0};

  reader.readI32(mode);
  reader.readI32(activebuffer);
  for (int i = 0; i < 6; ++i) {
    reader.readI32(usebeats[i]);
    reader.readI32(delay[i]);
  }

  return std::make_unique<avs::UnknownRenderObjectEffect>("Trans / Multi Delay", entry.payload);
}

//=============================================================================
// Trans / Video Delay (APE)
//=============================================================================
std::unique_ptr<avs::Effect> makeVideoDelay(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  (void)preset;
  PayloadReader reader(entry.payload);

  std::int32_t enabled = 0, usebeats = 0, delay = 0;

  reader.readI32(enabled);
  reader.readI32(usebeats);
  reader.readI32(delay);

  return std::make_unique<avs::UnknownRenderObjectEffect>("Trans / Video Delay", entry.payload);
}

AVS_LEGACY_REGISTER_EFFECT("Trans / Channel Shift", makeChannelShift);
AVS_LEGACY_REGISTER_EFFECT("Trans / Color Reduction", makeColorReduction);
AVS_LEGACY_REGISTER_EFFECT("Trans / Multiplier", makeMultiplier);
AVS_LEGACY_REGISTER_EFFECT("Trans / Multi Delay", makeMultiDelay);
AVS_LEGACY_REGISTER_EFFECT("Trans / Video Delay", makeVideoDelay);

//=============================================================================
// Misc / Beat Hold (Laser)
//=============================================================================
std::unique_ptr<avs::Effect> makeBeatHold(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  (void)preset;
  PayloadReader reader(entry.payload);

  std::int32_t decayMS = 0, beatSkip = 0;

  reader.readI32(decayMS);
  reader.readI32(beatSkip);

  return std::make_unique<avs::UnknownRenderObjectEffect>("Misc / Beat Hold", entry.payload);
}

//=============================================================================
// Render / Brennan's Effect (Laser)
//=============================================================================
std::unique_ptr<avs::Effect> makeBrennan(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  // No configuration data - returns empty config
  (void)preset;
  return std::make_unique<avs::UnknownRenderObjectEffect>("Render / Brennan's Effect", entry.payload);
}

//=============================================================================
// Render / Moving Cone (Laser)
//=============================================================================
std::unique_ptr<avs::Effect> makeMovingCone(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  (void)preset;
  PayloadReader reader(entry.payload);

  // Variable-length format: num_colors + colors array + 6 int32s
  std::int32_t num_colors = 0;
  reader.readI32(num_colors);

  // Skip color array (variable length, up to 16 colors)
  for (int i = 0; i < num_colors && i < 16; ++i) {
    std::int32_t color = 0;
    reader.readI32(color);
  }

  std::int32_t maxdist = 0, size = 0, size2 = 0, num_seg = 0, mode = 0, maxdist2 = 0;
  reader.readI32(maxdist);
  reader.readI32(size);
  reader.readI32(size2);
  reader.readI32(num_seg);
  reader.readI32(mode);
  reader.readI32(maxdist2);

  return std::make_unique<avs::UnknownRenderObjectEffect>("Render / Moving Cone", entry.payload);
}

//=============================================================================
// Render / Moving Line (Laser)
//=============================================================================
std::unique_ptr<avs::Effect> makeMovingLine(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  (void)preset;
  PayloadReader reader(entry.payload);

  // Variable-length format: num_colors + colors array + 4 int32s
  std::int32_t num_colors = 0;
  reader.readI32(num_colors);

  // Skip color array (variable length, up to 16 colors)
  for (int i = 0; i < num_colors && i < 16; ++i) {
    std::int32_t color = 0;
    reader.readI32(color);
  }

  std::int32_t maxdist = 0, size = 0, size2 = 0, maxbeatcnt = 0;
  reader.readI32(maxdist);
  reader.readI32(size);
  reader.readI32(size2);
  reader.readI32(maxbeatcnt);

  return std::make_unique<avs::UnknownRenderObjectEffect>("Render / Moving Line", entry.payload);
}

//=============================================================================
// Misc / Transform (Laser)
//=============================================================================
std::unique_ptr<avs::Effect> makeTransform(const LegacyEffectEntry& entry, ParsedPreset& preset) {
  (void)preset;
  // Complex variable-length format with EEL expressions
  // Format: int32 rectangular + byte version_flag + variable strings
  // Just preserve binary data as this is complex
  return std::make_unique<avs::UnknownRenderObjectEffect>("Misc / Transform", entry.payload);
}

AVS_LEGACY_REGISTER_EFFECT("Misc / Beat Hold", makeBeatHold);
AVS_LEGACY_REGISTER_EFFECT("Render / Brennan's Effect", makeBrennan);
AVS_LEGACY_REGISTER_EFFECT("Render / Moving Cone", makeMovingCone);
AVS_LEGACY_REGISTER_EFFECT("Render / Moving Line", makeMovingLine);
AVS_LEGACY_REGISTER_EFFECT("Misc / Transform", makeTransform);

}  // namespace avs::effects::legacy
