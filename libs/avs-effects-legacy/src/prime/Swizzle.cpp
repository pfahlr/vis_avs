#include <avs/effects/prime/Swizzle.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <string>

namespace {

avs::effects::SwizzleMode parseMode(const std::string& token) {
  std::string lower(token);
  std::transform(lower.begin(), lower.end(), lower.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  if (lower == "rgb") return avs::effects::SwizzleMode::RGB;
  if (lower == "rbg") return avs::effects::SwizzleMode::RBG;
  if (lower == "grb") return avs::effects::SwizzleMode::GRB;
  if (lower == "gbr") return avs::effects::SwizzleMode::GBR;
  if (lower == "brg") return avs::effects::SwizzleMode::BRG;
  if (lower == "bgr") return avs::effects::SwizzleMode::BGR;
  return avs::effects::SwizzleMode::RGB;
}

std::array<std::uint8_t, 3> orderForMode(avs::effects::SwizzleMode mode) {
  switch (mode) {
    case avs::effects::SwizzleMode::RGB:
      return {0, 1, 2};
    case avs::effects::SwizzleMode::RBG:
      return {0, 2, 1};
    case avs::effects::SwizzleMode::GRB:
      return {1, 0, 2};
    case avs::effects::SwizzleMode::GBR:
      return {1, 2, 0};
    case avs::effects::SwizzleMode::BRG:
      return {2, 0, 1};
    case avs::effects::SwizzleMode::BGR:
      return {2, 1, 0};
  }
  return {0, 1, 2};
}
}  // namespace

namespace avs::effects {

bool Swizzle::render(avs::core::RenderContext& context) {
  if (!context.framebuffer.data || context.framebuffer.size == 0) {
    return true;
  }
  std::uint8_t* data = context.framebuffer.data;
  const std::size_t size = context.framebuffer.size;
  for (std::size_t offset = 0; offset + 3 < size; offset += 4) {
    const std::uint8_t original[3] = {data[offset + 0], data[offset + 1], data[offset + 2]};
    data[offset + 0] = original[order_[0]];
    data[offset + 1] = original[order_[1]];
    data[offset + 2] = original[order_[2]];
  }
  return true;
}

void Swizzle::setParams(const avs::core::ParamBlock& params) {
  const std::string modeToken = params.getString("mode", params.getString("order", "rgb"));
  mode_ = parseMode(modeToken);
  order_ = orderForMode(mode_);
}

}  // namespace avs::effects
