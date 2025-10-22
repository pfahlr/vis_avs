#include <avs/effects/blend_ops.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <string>

namespace avs::effects {
namespace {

inline std::uint8_t saturatingAdd(std::uint8_t a, std::uint8_t b) {
  const std::uint16_t sum = static_cast<std::uint16_t>(a) + static_cast<std::uint16_t>(b);
  const std::uint16_t mask = static_cast<std::uint16_t>(-static_cast<std::int16_t>(sum > 255));
  return static_cast<std::uint8_t>((sum & 0xFFu) | (mask & 0xFFu));
}

inline std::uint8_t average(std::uint8_t a, std::uint8_t b) {
  const std::uint16_t sum = static_cast<std::uint16_t>(a) + static_cast<std::uint16_t>(b);
  return static_cast<std::uint8_t>(sum >> 1);
}

inline std::uint8_t blendTable(std::uint8_t value, std::uint8_t weight) {
  return static_cast<std::uint8_t>(
      (static_cast<std::uint32_t>(value) * static_cast<std::uint32_t>(weight)) / 255u);
}

inline std::uint8_t blendAdjust(std::uint8_t dst, std::uint8_t src, std::uint8_t alpha) {
  const std::uint8_t inv = static_cast<std::uint8_t>(255u - alpha);
  return static_cast<std::uint8_t>(blendTable(src, alpha) + blendTable(dst, inv));
}

inline std::uint8_t channelMax(std::uint8_t a, std::uint8_t b) {
  const std::uint8_t mask = static_cast<std::uint8_t>(-static_cast<std::int8_t>(a < b));
  return static_cast<std::uint8_t>(a ^ ((a ^ b) & mask));
}

inline std::uint8_t channelMin(std::uint8_t a, std::uint8_t b) {
  const std::uint8_t mask = static_cast<std::uint8_t>(-static_cast<std::int8_t>(a < b));
  return static_cast<std::uint8_t>(b ^ ((a ^ b) & mask));
}

template <typename Func>
inline void applyChannels(std::uint8_t* dst, const std::uint8_t* src, Func&& func) {
  dst[0] = func(dst[0], src[0]);
  dst[1] = func(dst[1], src[1]);
  dst[2] = func(dst[2], src[2]);
  dst[3] = func(dst[3], src[3]);
}

void applyReplace(const BlendConfig&, std::uint8_t* dst, const std::uint8_t* src) {
  dst[0] = src[0];
  dst[1] = src[1];
  dst[2] = src[2];
  dst[3] = src[3];
}

void applyAdditive(const BlendConfig&, std::uint8_t* dst, const std::uint8_t* src) {
  applyChannels(dst, src, [](std::uint8_t d, std::uint8_t s) { return saturatingAdd(d, s); });
}

void applyBlend(const BlendConfig&, std::uint8_t* dst, const std::uint8_t* src) {
  applyChannels(dst, src, [](std::uint8_t d, std::uint8_t s) { return average(d, s); });
}

void applyAlpha(const BlendConfig& config, std::uint8_t* dst, const std::uint8_t* src) {
  const std::uint8_t alpha = config.alpha;
  applyChannels(dst, src,
                [alpha](std::uint8_t d, std::uint8_t s) { return blendAdjust(d, s, alpha); });
}

void applyAlpha2(const BlendConfig& config, std::uint8_t* dst, const std::uint8_t* src) {
  const std::uint8_t alpha = config.alpha2;
  applyChannels(dst, src,
                [alpha](std::uint8_t d, std::uint8_t s) { return blendAdjust(d, s, alpha); });
}

void applyAlphaSlide(const BlendConfig& config, std::uint8_t* dst, const std::uint8_t* src) {
  const std::uint8_t alpha = config.slide;
  applyChannels(dst, src,
                [alpha](std::uint8_t d, std::uint8_t s) { return blendAdjust(d, s, alpha); });
}

void applyAbove(const BlendConfig&, std::uint8_t* dst, const std::uint8_t* src) {
  applyChannels(dst, src, [](std::uint8_t d, std::uint8_t s) { return channelMax(d, s); });
}

void applyBelow(const BlendConfig&, std::uint8_t* dst, const std::uint8_t* src) {
  applyChannels(dst, src, [](std::uint8_t d, std::uint8_t s) { return channelMin(d, s); });
}

using PixelFunc = void (*)(const BlendConfig&, std::uint8_t*, const std::uint8_t*);

constexpr std::array<PixelFunc, 11> kDispatch = {
    applyAdditive,    // Additive
    applyAlpha,       // Alpha
    applyAlpha2,      // Alpha2
    applyAlphaSlide,  // AlphaSlide
    applyBlend,       // Blend
    applyAlphaSlide,  // BlendSlide
    applyReplace,     // Replace
    applyBlend,       // DefaultBlend
    applyBlend,       // DefrendBlend
    applyAbove,       // Above
    applyBelow,       // Below
};

static_assert(static_cast<std::size_t>(BlendOp::Below) == kDispatch.size() - 1,
              "BlendOp dispatch table out of sync");

std::string toLower(std::string_view token) {
  std::string result(token);
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return result;
}

}  // namespace

std::optional<BlendOp> parseBlendOpToken(std::string_view token) {
  const std::string lower = toLower(token);
  if (lower == "add" || lower == "additive") {
    return BlendOp::Additive;
  }
  if (lower == "alpha") {
    return BlendOp::Alpha;
  }
  if (lower == "alpha2" || lower == "alpha_2") {
    return BlendOp::Alpha2;
  }
  if (lower == "alphaslide" || lower == "alpha_slide" || lower == "alpha-slide") {
    return BlendOp::AlphaSlide;
  }
  if (lower == "blend") {
    return BlendOp::Blend;
  }
  if (lower == "blendslide" || lower == "blend_slide" || lower == "blend-slide") {
    return BlendOp::BlendSlide;
  }
  if (lower == "replace") {
    return BlendOp::Replace;
  }
  if (lower == "default" || lower == "defaultblend" || lower == "defblend") {
    return BlendOp::DefaultBlend;
  }
  if (lower == "defrend" || lower == "defrendblend" || lower == "defaultrenderblend" ||
      lower == "renderblend") {
    return BlendOp::DefrendBlend;
  }
  if (lower == "above") {
    return BlendOp::Above;
  }
  if (lower == "below") {
    return BlendOp::Below;
  }
  return std::nullopt;
}

BlendOp parseBlendOpOrDefault(std::string_view token, BlendOp fallback) {
  if (auto op = parseBlendOpToken(token)) {
    return *op;
  }
  return fallback;
}

void blendPixelInPlace(BlendOp op, const BlendConfig& config, std::uint8_t* dst,
                       const std::uint8_t* src) {
  const auto index = static_cast<std::size_t>(op);
  kDispatch[index](config, dst, src);
}

std::array<std::uint8_t, 4> blendPixel(BlendOp op, const BlendConfig& config,
                                       const std::array<std::uint8_t, 4>& dst,
                                       const std::array<std::uint8_t, 4>& src) {
  auto result = dst;
  const auto index = static_cast<std::size_t>(op);
  kDispatch[index](config, result.data(), src.data());
  return result;
}

}  // namespace avs::effects
