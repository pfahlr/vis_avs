#include "avs/effects/render/effect_oscilloscope_star.h"

#include <algorithm>

#include "avs/effects/effect_registry.hpp"

namespace avs::effects::render {

AVS_EFFECT_TOKEN("Render / Oscilloscope Star");
REGISTER_AVS_EFFECT(EffectOscilloscopeStar, "Render / Oscilloscope Star");

namespace {
constexpr std::size_t kFieldSize = sizeof(std::uint32_t);

std::uint32_t readU32(const std::uint8_t* data) {
  return static_cast<std::uint32_t>(data[0]) |
         (static_cast<std::uint32_t>(data[1]) << 8) |
         (static_cast<std::uint32_t>(data[2]) << 16) |
         (static_cast<std::uint32_t>(data[3]) << 24);
}

void writeU32(std::uint32_t value, std::vector<std::uint8_t>& out) {
  out.push_back(static_cast<std::uint8_t>(value & 0xFFu));
  out.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFFu));
  out.push_back(static_cast<std::uint8_t>((value >> 16) & 0xFFu));
  out.push_back(static_cast<std::uint8_t>((value >> 24) & 0xFFu));
}

}  // namespace

EffectOscilloscopeStar::EffectOscilloscopeStar() {
  effect_ = 0 | (2u << 2) | (2u << 4);
  numColors_ = 1;
  colors_.fill(0);
  colors_[0] = 0x00FFFFFFu;
  size_ = 8;
  rotation_ = 3;
}

void EffectOscilloscopeStar::render(LegacyRenderContext& context) {
  (void)context;
  // TODO: Implement oscilloscope star rendering.
}

void EffectOscilloscopeStar::loadConfig(const std::uint8_t* data, std::size_t size) {
  if (data == nullptr || size == 0) {
    return;
  }

  std::size_t pos = 0;
  auto readField = [&](std::uint32_t& target) {
    if (pos + kFieldSize > size) {
      return false;
    }
    target = readU32(data + pos);
    pos += kFieldSize;
    return true;
  };

  readField(effect_);

  std::uint32_t colors = numColors_;
  if (readField(colors)) {
    if (colors > kMaxColors) {
      numColors_ = 1;
    } else {
      numColors_ = colors;
    }
  }

  std::fill(colors_.begin(), colors_.end(), 0u);
  std::size_t toRead = std::min<std::size_t>(numColors_, kMaxColors);
  for (std::size_t i = 0; i < toRead; ++i) {
    if (pos + kFieldSize > size) {
      numColors_ = static_cast<std::uint32_t>(i);
      break;
    }
    colors_[i] = readU32(data + pos);
    pos += kFieldSize;
  }
  if (numColors_ == 0) {
    numColors_ = 1;
    colors_[0] = 0x00FFFFFFu;
  }

  readField(size_);
  readField(rotation_);
}

std::vector<std::uint8_t> EffectOscilloscopeStar::saveConfig() const {
  std::vector<std::uint8_t> buffer;
  buffer.reserve((2 + numColors_) * kFieldSize);
  writeU32(effect_, buffer);
  writeU32(std::min<std::uint32_t>(numColors_, static_cast<std::uint32_t>(kMaxColors)), buffer);
  for (std::size_t i = 0; i < std::min<std::size_t>(numColors_, kMaxColors); ++i) {
    writeU32(colors_[i], buffer);
  }
  writeU32(size_, buffer);
  writeU32(rotation_, buffer);
  return buffer;
}

}  // namespace avs::effects::render
