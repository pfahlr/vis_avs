#include "effects/trans/effect_roto_blitter.h"

#include <algorithm>
#include <cmath>

#include "avs/core/ParamBlock.hpp"

namespace {
constexpr float kPi = 3.14159265358979323846f;

bool readBoolLike(const avs::core::ParamBlock& params, const std::string& key, bool fallback) {
  if (!params.contains(key)) {
    return fallback;
  }
  bool value = params.getBool(key, fallback);
  value = params.getInt(key, value ? 1 : 0) != 0;
  return value;
}

}  // namespace

namespace avs::effects::trans {

std::array<float, 2> RotoBlitter::anchorFromToken(const std::string& token) {
  if (token == "center") return {0.5f, 0.5f};
  if (token == "top_left" || token == "topleft") return {0.0f, 0.0f};
  if (token == "top_right" || token == "topright") return {1.0f, 0.0f};
  if (token == "bottom_left" || token == "bottomleft") return {0.0f, 1.0f};
  if (token == "bottom_right" || token == "bottomright") return {1.0f, 1.0f};
  if (token == "center_left" || token == "centerleft") return {0.0f, 0.5f};
  if (token == "center_right" || token == "centerright") return {1.0f, 0.5f};
  if (token == "top_center" || token == "topcenter") return {0.5f, 0.0f};
  if (token == "bottom_center" || token == "bottomcenter") return {0.5f, 1.0f};
  return {0.0f, 0.0f};
}

int RotoBlitter::clampInt(int value, int minValue, int maxValue) {
  return std::max(minValue, std::min(maxValue, value));
}

float RotoBlitter::wrapCoord(float value, float size) {
  if (size <= 0.0f) {
    return 0.0f;
  }
  float wrapped = std::fmod(value, size);
  if (wrapped < 0.0f) {
    wrapped += size;
  }
  return wrapped;
}

int RotoBlitter::wrapIndex(int value, int size) {
  if (size <= 0) {
    return 0;
  }
  int wrapped = value % size;
  if (wrapped < 0) {
    wrapped += size;
  }
  return wrapped;
}

bool RotoBlitter::ensureHistory(const avs::core::RenderContext& context) {
  if (!context.framebuffer.data || context.width <= 0 || context.height <= 0) {
    return false;
  }
  const std::size_t expected = static_cast<std::size_t>(context.width) * static_cast<std::size_t>(context.height) * 4u;
  if (context.framebuffer.size < expected) {
    return false;
  }
  if (history_.size() != expected || historyWidth_ != context.width || historyHeight_ != context.height) {
    history_.assign(context.framebuffer.data, context.framebuffer.data + expected);
    historyWidth_ = context.width;
    historyHeight_ = context.height;
  }
  if (scratch_.size() != expected) {
    scratch_.resize(expected);
  }
  return true;
}

void RotoBlitter::storeHistory(const avs::core::RenderContext& context) {
  if (!context.framebuffer.data || context.width <= 0 || context.height <= 0) {
    return;
  }
  const std::size_t expected = static_cast<std::size_t>(context.width) * static_cast<std::size_t>(context.height) * 4u;
  if (context.framebuffer.size < expected) {
    return;
  }
  if (history_.size() != expected) {
    history_.resize(expected);
  }
  std::copy(context.framebuffer.data, context.framebuffer.data + expected, history_.begin());
  historyWidth_ = context.width;
  historyHeight_ = context.height;
}

std::array<std::uint8_t, 4> RotoBlitter::sampleNearest(float x, float y) const {
  if (history_.empty() || historyWidth_ <= 0 || historyHeight_ <= 0) {
    return {0, 0, 0, 255};
  }
  const float fx = wrapCoord(x, static_cast<float>(historyWidth_));
  const float fy = wrapCoord(y, static_cast<float>(historyHeight_));
  const int ix = wrapIndex(static_cast<int>(std::floor(fx)), historyWidth_);
  const int iy = wrapIndex(static_cast<int>(std::floor(fy)), historyHeight_);
  const std::size_t offset =
      (static_cast<std::size_t>(iy) * static_cast<std::size_t>(historyWidth_) + static_cast<std::size_t>(ix)) * 4u;
  if (offset + 3 >= history_.size()) {
    return {0, 0, 0, 255};
  }
  return {history_[offset + 0], history_[offset + 1], history_[offset + 2], history_[offset + 3]};
}

std::array<std::uint8_t, 4> RotoBlitter::sampleBilinear(float x, float y) const {
  if (history_.empty() || historyWidth_ <= 0 || historyHeight_ <= 0) {
    return {0, 0, 0, 255};
  }
  const float fx = wrapCoord(x, static_cast<float>(historyWidth_));
  const float fy = wrapCoord(y, static_cast<float>(historyHeight_));

  int x0 = static_cast<int>(std::floor(fx));
  int y0 = static_cast<int>(std::floor(fy));
  const float tx = fx - static_cast<float>(x0);
  const float ty = fy - static_cast<float>(y0);

  const int x1 = wrapIndex(x0 + 1, historyWidth_);
  const int y1 = wrapIndex(y0 + 1, historyHeight_);
  x0 = wrapIndex(x0, historyWidth_);
  y0 = wrapIndex(y0, historyHeight_);

  const auto fetch = [&](int px, int py) {
    const std::size_t base =
        (static_cast<std::size_t>(py) * static_cast<std::size_t>(historyWidth_) + static_cast<std::size_t>(px)) * 4u;
    return std::array<std::uint8_t, 4>{history_[base + 0], history_[base + 1], history_[base + 2], history_[base + 3]};
  };

  const auto c00 = fetch(x0, y0);
  const auto c10 = fetch(x1, y0);
  const auto c01 = fetch(x0, y1);
  const auto c11 = fetch(x1, y1);

  std::array<std::uint8_t, 4> result{};
  for (int i = 0; i < 4; ++i) {
    const float c0 = c00[static_cast<std::size_t>(i)] +
                     (c10[static_cast<std::size_t>(i)] - c00[static_cast<std::size_t>(i)]) * tx;
    const float c1 = c01[static_cast<std::size_t>(i)] +
                     (c11[static_cast<std::size_t>(i)] - c01[static_cast<std::size_t>(i)]) * tx;
    const float value = c0 + (c1 - c0) * ty;
    const float clamped = std::clamp(value, 0.0f, 255.0f);
    result[static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(std::round(clamped));
  }
  return result;
}

void RotoBlitter::setParams(const avs::core::ParamBlock& params) {
  const int previousBase = zoomBaseRaw_;
  int base = zoomBaseRaw_;
  if (params.contains("zoom_scale")) {
    base = clampInt(params.getInt("zoom_scale", base), 0, 256);
  }
  if (params.contains("zoom_base")) {
    base = clampInt(params.getInt("zoom_base", base), 0, 256);
  }
  if (params.contains("zoom")) {
    base = clampInt(params.getInt("zoom", base), 0, 256);
  }
  zoomBaseRaw_ = base;

  int beatZoom = zoomBeatRaw_;
  if (params.contains("zoom_scale2")) {
    beatZoom = clampInt(params.getInt("zoom_scale2", beatZoom), 0, 256);
  }
  if (params.contains("zoom_beat")) {
    beatZoom = clampInt(params.getInt("zoom_beat", beatZoom), 0, 256);
  }
  zoomBeatRaw_ = beatZoom;

  if (previousBase != zoomBaseRaw_) {
    zoomCurrentRaw_ = zoomBaseRaw_;
  }

  if (params.contains("rot_dir")) {
    rotationRaw_ = clampInt(params.getInt("rot_dir", rotationRaw_), 0, 64);
  }
  if (params.contains("rotation")) {
    rotationRaw_ = clampInt(params.getInt("rotation", rotationRaw_), 0, 64);
  }

  if (params.contains("beatch_speed")) {
    reverseSpeed_ = clampInt(params.getInt("beatch_speed", reverseSpeed_), 0, 8);
  }
  if (params.contains("reverse_speed")) {
    reverseSpeed_ = clampInt(params.getInt("reverse_speed", reverseSpeed_), 0, 8);
  }

  if (params.contains("blend")) {
    blend_ = readBoolLike(params, "blend", blend_);
  }
  if (params.contains("subpixel")) {
    subpixel_ = readBoolLike(params, "subpixel", subpixel_);
  }

  if (params.contains("beatch")) {
    beatReverse_ = readBoolLike(params, "beatch", beatReverse_);
  }
  if (params.contains("beat_reverse")) {
    beatReverse_ = readBoolLike(params, "beat_reverse", beatReverse_);
  }
  if (!beatReverse_) {
    reverseDirection_ = 1;
    reversePos_ = 1.0f;
  }

  if (params.contains("beatch_scale")) {
    beatZoom_ = readBoolLike(params, "beatch_scale", beatZoom_);
  }
  if (params.contains("beat_zoom")) {
    beatZoom_ = readBoolLike(params, "beat_zoom", beatZoom_);
  }
  if (!beatZoom_) {
    zoomCurrentRaw_ = zoomBaseRaw_;
  }

  if (params.contains("anchor")) {
    anchorNorm_ = anchorFromToken(params.getString("anchor", ""));
  }
  if (params.contains("anchor_mode")) {
    anchorNorm_ = anchorFromToken(params.getString("anchor_mode", ""));
  }
  if (params.getBool("center", false)) {
    anchorNorm_ = {0.5f, 0.5f};
  }
  if (params.contains("anchor_x") || params.contains("anchor_y")) {
    anchorNorm_[0] = std::clamp(params.getFloat("anchor_x", anchorNorm_[0]), 0.0f, 1.0f);
    anchorNorm_[1] = std::clamp(params.getFloat("anchor_y", anchorNorm_[1]), 0.0f, 1.0f);
  }
}

bool RotoBlitter::render(avs::core::RenderContext& context) {
  if (!ensureHistory(context)) {
    return true;
  }

  if (context.audioBeat && beatReverse_) {
    reverseDirection_ = -reverseDirection_;
  }
  if (!beatReverse_) {
    reverseDirection_ = 1;
  }
  const float targetDirection = static_cast<float>(reverseDirection_);
  const float smoothing = 1.0f / (1.0f + static_cast<float>(reverseSpeed_) * 4.0f);
  reversePos_ += (targetDirection - reversePos_) * smoothing;
  if ((targetDirection > 0.0f && reversePos_ > targetDirection) ||
      (targetDirection < 0.0f && reversePos_ < targetDirection)) {
    reversePos_ = targetDirection;
  }

  if (beatZoom_ && context.audioBeat) {
    zoomCurrentRaw_ = zoomBeatRaw_;
  }

  zoomBaseRaw_ = clampInt(zoomBaseRaw_, 0, 256);
  zoomBeatRaw_ = clampInt(zoomBeatRaw_, 0, 256);
  zoomCurrentRaw_ = clampInt(zoomCurrentRaw_, 0, 256);

  int fVal = zoomCurrentRaw_;
  if (zoomBaseRaw_ < zoomBeatRaw_) {
    fVal = std::max(zoomCurrentRaw_, zoomBaseRaw_);
    if (zoomCurrentRaw_ > zoomBaseRaw_) {
      zoomCurrentRaw_ = std::max(zoomCurrentRaw_ - 3, zoomBaseRaw_);
    }
  } else {
    fVal = std::min(zoomCurrentRaw_, zoomBaseRaw_);
    if (zoomCurrentRaw_ < zoomBaseRaw_) {
      zoomCurrentRaw_ = std::min(zoomCurrentRaw_ + 3, zoomBaseRaw_);
    }
  }
  fVal = clampInt(fVal, 0, 256);
  const float zoom = 1.0f + (static_cast<float>(fVal) - 31.0f) / 31.0f;

  const float angleDegrees = (static_cast<float>(rotationRaw_) - 32.0f) * reversePos_;
  const float angleRadians = angleDegrees * (kPi / 180.0f);
  const float cosTheta = std::cos(angleRadians) * zoom;
  const float sinTheta = std::sin(angleRadians) * zoom;

  const float anchorX = anchorNorm_[0] * static_cast<float>(std::max(context.width - 1, 0));
  const float anchorY = anchorNorm_[1] * static_cast<float>(std::max(context.height - 1, 0));

  const std::size_t expected = static_cast<std::size_t>(context.width) * static_cast<std::size_t>(context.height) * 4u;
  for (int y = 0; y < context.height; ++y) {
    for (int x = 0; x < context.width; ++x) {
      const float dx = static_cast<float>(x) - anchorX;
      const float dy = static_cast<float>(y) - anchorY;
      const float sampleX = dx * cosTheta - dy * sinTheta + anchorX;
      const float sampleY = dx * sinTheta + dy * cosTheta + anchorY;
      const auto color = subpixel_ ? sampleBilinear(sampleX, sampleY) : sampleNearest(sampleX, sampleY);

      const std::size_t offset =
          (static_cast<std::size_t>(y) * static_cast<std::size_t>(context.width) + static_cast<std::size_t>(x)) * 4u;
      if (offset + 3 >= expected) {
        continue;
      }
      const std::uint8_t* input = context.framebuffer.data + offset;
      std::uint8_t* out = scratch_.data() + offset;
      if (blend_) {
        out[0] = static_cast<std::uint8_t>((static_cast<int>(input[0]) + static_cast<int>(color[0])) / 2);
        out[1] = static_cast<std::uint8_t>((static_cast<int>(input[1]) + static_cast<int>(color[1])) / 2);
        out[2] = static_cast<std::uint8_t>((static_cast<int>(input[2]) + static_cast<int>(color[2])) / 2);
        out[3] = static_cast<std::uint8_t>((static_cast<int>(input[3]) + static_cast<int>(color[3])) / 2);
      } else {
        out[0] = color[0];
        out[1] = color[1];
        out[2] = color[2];
        out[3] = color[3];
      }
    }
  }

  std::copy(scratch_.begin(), scratch_.begin() + expected, context.framebuffer.data);
  storeHistory(context);
  return true;
}

}  // namespace avs::effects::trans

