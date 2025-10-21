#include "effects/trans/effect_channel_shift.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <string>

#include "avs/core/RenderContext.hpp"

namespace {
constexpr int kIdRgb = 1183;
constexpr int kIdRbg = 1020;
constexpr int kIdGbr = 1018;
constexpr int kIdGrb = 1022;
constexpr int kIdBrg = 1019;
constexpr int kIdBgr = 1021;

std::string toLower(std::string token) {
  std::transform(token.begin(), token.end(), token.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return token;
}

}  // namespace

namespace avs::effects::trans {

ChannelShift::ChannelShift()
    : configuredMode_(Mode::RBG),
      currentMode_(Mode::RBG),
      channelOrder_(orderForMode(Mode::RBG)),
      randomizeOnBeat_(true) {}

void ChannelShift::setParams(const avs::core::ParamBlock& params) {
  Mode targetMode = configuredMode_;

  if (params.contains("mode")) {
    const std::string modeToken = params.getString("mode", "");
    if (!modeToken.empty()) {
      targetMode = modeFromString(modeToken, targetMode);
    } else {
      const int idValue = params.getInt("mode", idForMode(targetMode));
      targetMode = modeFromId(idValue, targetMode);
    }
  }

  const std::string orderToken = params.getString("order", "");
  if (!orderToken.empty()) {
    targetMode = modeFromString(orderToken, targetMode);
  }

  configuredMode_ = targetMode;
  setMode(targetMode);

  randomizeOnBeat_ = params.getBool("onbeat", randomizeOnBeat_);
}

bool ChannelShift::render(avs::core::RenderContext& context) {
  if (!context.framebuffer.data || context.framebuffer.size < 4) {
    return true;
  }

  if (context.audioBeat && randomizeOnBeat_) {
    const std::uint32_t randomValue = context.rng.nextUint32();
    const std::size_t index = static_cast<std::size_t>(randomValue % kBeatModes.size());
    setMode(kBeatModes[index]);
  } else if (!randomizeOnBeat_ && currentMode_ != configuredMode_) {
    setMode(configuredMode_);
  }

  if (currentMode_ == Mode::RGB) {
    return true;
  }

  std::uint8_t* data = context.framebuffer.data;
  const std::size_t size = context.framebuffer.size;
  for (std::size_t offset = 0; offset + 3 < size; offset += 4) {
    const std::array<std::uint8_t, 3> original = {data[offset + 0], data[offset + 1], data[offset + 2]};
    data[offset + 0] = original[channelOrder_[0]];
    data[offset + 1] = original[channelOrder_[1]];
    data[offset + 2] = original[channelOrder_[2]];
  }

  return true;
}

void ChannelShift::setMode(Mode mode) {
  currentMode_ = mode;
  channelOrder_ = orderForMode(mode);
}

ChannelShift::Mode ChannelShift::modeFromId(int id, Mode fallback) {
  switch (id) {
    case kIdRgb:
      return Mode::RGB;
    case kIdRbg:
      return Mode::RBG;
    case kIdGbr:
      return Mode::GBR;
    case kIdGrb:
      return Mode::GRB;
    case kIdBrg:
      return Mode::BRG;
    case kIdBgr:
      return Mode::BGR;
    default:
      return fallback;
  }
}

ChannelShift::Mode ChannelShift::modeFromString(const std::string& token, Mode fallback) {
  if (token.empty()) {
    return fallback;
  }
  const std::string lowered = toLower(token);
  if (lowered == "rgb") {
    return Mode::RGB;
  }
  if (lowered == "rbg") {
    return Mode::RBG;
  }
  if (lowered == "gbr") {
    return Mode::GBR;
  }
  if (lowered == "grb") {
    return Mode::GRB;
  }
  if (lowered == "brg") {
    return Mode::BRG;
  }
  if (lowered == "bgr") {
    return Mode::BGR;
  }
  return fallback;
}

std::array<std::uint8_t, 3> ChannelShift::orderForMode(Mode mode) {
  switch (mode) {
    case Mode::RGB:
      return {0u, 1u, 2u};
    case Mode::RBG:
      return {0u, 2u, 1u};
    case Mode::GBR:
      return {1u, 2u, 0u};
    case Mode::GRB:
      return {1u, 0u, 2u};
    case Mode::BRG:
      return {2u, 0u, 1u};
    case Mode::BGR:
      return {2u, 1u, 0u};
  }
  return {0u, 1u, 2u};
}

int ChannelShift::idForMode(Mode mode) {
  switch (mode) {
    case Mode::RGB:
      return kIdRgb;
    case Mode::RBG:
      return kIdRbg;
    case Mode::GBR:
      return kIdGbr;
    case Mode::GRB:
      return kIdGrb;
    case Mode::BRG:
      return kIdBrg;
    case Mode::BGR:
      return kIdBgr;
  }
  return kIdRgb;
}

}  // namespace avs::effects::trans

