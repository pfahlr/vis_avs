#include "effects/misc/effect_render_mode.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <string_view>

#include <avs/core/ParamBlock.hpp>
#include <avs/runtime/GlobalState.hpp>

namespace avs::effects::misc {
namespace {

constexpr std::uint32_t kEnableBit = 0x80000000u;
constexpr std::uint8_t kModeMask = 0xFFu;
constexpr std::uint8_t kAlphaMask = 0xFFu;
constexpr std::uint8_t kLineWidthMask = 0xFFu;

struct ModeToken {
  const char* name;
  std::uint8_t mode;
};

constexpr std::array<ModeToken, 16> kModeTokens = {{{"replace", 0},
                                                    {"add", 1},
                                                    {"additive", 1},
                                                    {"maximum", 2},
                                                    {"max", 2},
                                                    {"50/50", 3},
                                                    {"50-50", 3},
                                                    {"half", 3},
                                                    {"sub1", 4},
                                                    {"subtractive", 4},
                                                    {"subtractive1", 4},
                                                    {"sub2", 5},
                                                    {"subtractive2", 5},
                                                    {"multiply", 6},
                                                    {"adjustable", 7},
                                                    {"xor", 8}}};

std::string toLower(std::string_view token) {
  std::string result(token);
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return result;
}

std::uint8_t clampByte(int value) {
  return static_cast<std::uint8_t>(std::clamp(value, 0, 255));
}

}  // namespace

RenderMode::RenderMode() = default;

bool RenderMode::render(avs::core::RenderContext& context) {
  if (!context.globals) {
    return true;
  }
  if (!enabled_) {
    return true;
  }
  const std::uint32_t raw = encodeRaw() & ~kEnableBit;
  context.globals->legacyRender.lineBlendMode = raw;
  context.globals->legacyRender.lineBlendModeActive = true;
  return true;
}

void RenderMode::setParams(const avs::core::ParamBlock& params) {
  if (params.contains("value")) {
    applyRaw(static_cast<std::uint32_t>(params.getInt("value", static_cast<int>(encodeRaw()))));
  }
  if (params.contains("raw")) {
    applyRaw(static_cast<std::uint32_t>(params.getInt("raw", static_cast<int>(encodeRaw()))));
  }
  if (params.contains("enabled")) {
    enabled_ = params.getBool("enabled", enabled_);
  }
  if (params.contains("mode")) {
    const std::string token = params.getString("mode", "");
    if (!token.empty()) {
      setModeFromString(token);
    } else {
      setModeFromInt(params.getInt("mode", mode_));
    }
  }
  if (params.contains("mode_name")) {
    setModeFromString(params.getString("mode_name", ""));
  }
  if (params.contains("blend_mode")) {
    setModeFromString(params.getString("blend_mode", ""));
  }
  if (params.contains("alpha")) {
    adjustableAlpha_ = clampByte(params.getInt("alpha", adjustableAlpha_));
  }
  if (params.contains("blend_alpha")) {
    adjustableAlpha_ = clampByte(params.getInt("blend_alpha", adjustableAlpha_));
  }
  if (params.contains("adjust")) {
    adjustableAlpha_ = clampByte(params.getInt("adjust", adjustableAlpha_));
  }
  if (params.contains("adjustment")) {
    adjustableAlpha_ = clampByte(params.getInt("adjustment", adjustableAlpha_));
  }

  auto updateWidth = [&](const std::string& key) {
    if (params.contains(key)) {
      lineWidth_ = clampByte(params.getInt(key, lineWidth_));
      return true;
    }
    return false;
  };
  if (!updateWidth("line_width")) {
    if (!updateWidth("linewidth")) {
      if (!updateWidth("line_size")) {
        if (!updateWidth("linesize")) {
          updateWidth("thickness");
        }
      }
    }
  }
}

void RenderMode::applyRaw(std::uint32_t rawValue) {
  enabled_ = (rawValue & kEnableBit) != 0u;
  mode_ = static_cast<std::uint8_t>(rawValue & kModeMask);
  adjustableAlpha_ = static_cast<std::uint8_t>((rawValue >> 8) & kAlphaMask);
  lineWidth_ = static_cast<std::uint8_t>((rawValue >> 16) & kLineWidthMask);
}

std::uint32_t RenderMode::encodeRaw() const {
  std::uint32_t value = static_cast<std::uint32_t>(mode_ & kModeMask);
  value |= static_cast<std::uint32_t>(adjustableAlpha_) << 8;
  value |= static_cast<std::uint32_t>(lineWidth_) << 16;
  if (enabled_) {
    value |= kEnableBit;
  }
  return value;
}

void RenderMode::setModeFromInt(int modeValue) {
  mode_ = static_cast<std::uint8_t>(std::clamp(modeValue, 0, 9));
}

void RenderMode::setModeFromString(const std::string& token) {
  const std::string lowered = toLower(token);
  for (const auto& entry : kModeTokens) {
    if (lowered == entry.name) {
      mode_ = entry.mode;
      return;
    }
  }
  if (lowered == "minimum" || lowered == "min") {
    mode_ = 9;
  } else if (lowered == "maximum blend" || lowered == "max blend" || lowered == "maximumblend") {
    mode_ = 2;
  } else if (lowered == "subtractive blend 1") {
    mode_ = 4;
  } else if (lowered == "subtractive blend 2") {
    mode_ = 5;
  } else if (lowered == "50/50 blend" || lowered == "fifty" || lowered == "fiftyfifty") {
    mode_ = 3;
  }
}

}  // namespace avs::effects::misc

