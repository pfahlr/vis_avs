#include "avs/effects/trans/effect_fadeout.h"
#include <algorithm>
#include <cstddef>

#include "avs/effects/effect_registry.hpp"

namespace avs::effects::trans {

// NOTE: This effect uses the LegacyEffect interface which is incompatible with
// the development-branch EffectRegistry that expects IEffect-derived classes.
// To register this effect, it needs to be adapted to implement IEffect or
// wrapped in an adapter factory function (see effect_registry.cpp for examples).


namespace {
constexpr std::size_t kFieldSize = sizeof(std::uint32_t);

std::uint32_t readU32(const std::uint8_t* ptr) {
  return static_cast<std::uint32_t>(ptr[0]) |
         (static_cast<std::uint32_t>(ptr[1]) << 8) |
         (static_cast<std::uint32_t>(ptr[2]) << 16) |
         (static_cast<std::uint32_t>(ptr[3]) << 24);
}

void writeU32(std::uint32_t value, std::vector<std::uint8_t>& buffer) {
  buffer.push_back(static_cast<std::uint8_t>(value & 0xFFu));
  buffer.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFFu));
  buffer.push_back(static_cast<std::uint8_t>((value >> 16) & 0xFFu));
  buffer.push_back(static_cast<std::uint8_t>((value >> 24) & 0xFFu));
}

}  // namespace

EffectFadeout::EffectFadeout() = default;

void EffectFadeout::render(LegacyRenderContext& context) {
  (void)context;
  // TODO: Port legacy fadeout rendering algorithm.
}

void EffectFadeout::loadConfig(const std::uint8_t* data, std::size_t size) {
  fadelen_ = 16u;
  color_ = 0u;
  if (data == nullptr || size < kFieldSize) {
    return;
  }
  std::size_t offset = 0;
  if (offset + kFieldSize <= size) {
    fadelen_ = readU32(data + offset);
    offset += kFieldSize;
  }
  if (offset + kFieldSize <= size) {
    color_ = readU32(data + offset);
  }
  fadelen_ = std::min(fadelen_, 255u);
}

std::vector<std::uint8_t> EffectFadeout::saveConfig() const {
  std::vector<std::uint8_t> buffer;
  buffer.reserve(2 * kFieldSize);
  writeU32(fadelen_, buffer);
  writeU32(color_, buffer);
  return buffer;
}

}  // namespace avs::effects::trans
