#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "avs/effects/legacy_effect.h"

namespace avs::effects::misc {

class EffectUnknownObject : public LegacyEffect {
 public:
  EffectUnknownObject() = default;

  void render(LegacyRenderContext& context) override;
  void loadConfig(const std::uint8_t* data, std::size_t size) override;
  std::vector<std::uint8_t> saveConfig() const override;

  void setOriginalToken(std::string token);
  const std::string& originalToken() const noexcept { return originalToken_; }

 private:
  std::vector<std::uint8_t> rawConfig_;
  std::string originalToken_;
};

}  // namespace avs::effects::misc
