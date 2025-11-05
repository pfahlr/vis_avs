#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace avs::effects {

struct LegacyRenderContext {
  bool isBeat = false;
};

class LegacyEffect {
 public:
  virtual ~LegacyEffect() = default;

  virtual void render(LegacyRenderContext& context) = 0;

  virtual void loadConfig(const std::uint8_t* data, std::size_t size) = 0;

  virtual std::vector<std::uint8_t> saveConfig() const = 0;
};

}  // namespace avs::effects
