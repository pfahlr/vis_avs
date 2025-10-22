#pragma once

#include <array>
#include <cstdint>
#include <string>

#include <avs/core/IEffect.hpp>
#include <avs/core/ParamBlock.hpp>

namespace avs::effects::trans {

class ChannelShift : public avs::core::IEffect {
 public:
  ChannelShift();
  ~ChannelShift() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  enum class Mode {
    RGB,
    RBG,
    GBR,
    GRB,
    BRG,
    BGR,
  };

  static constexpr std::array<Mode, 6> kBeatModes = {
      Mode::RGB, Mode::RBG, Mode::GBR, Mode::GRB, Mode::BRG, Mode::BGR};

  void setMode(Mode mode);

  static Mode modeFromId(int id, Mode fallback);
  static Mode modeFromString(const std::string& token, Mode fallback);
  static std::array<std::uint8_t, 3> orderForMode(Mode mode);
  static int idForMode(Mode mode);

  Mode configuredMode_;
  Mode currentMode_;
  std::array<std::uint8_t, 3> channelOrder_;
  bool randomizeOnBeat_;
};

}  // namespace avs::effects::trans

