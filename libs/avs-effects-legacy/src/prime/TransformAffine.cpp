#include <avs/effects/TransformAffine.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include <avs/core/ParamBlock.hpp>
#include <avs/effects/legacy/gating.h>
#include <avs/effects/transform_affine.h>

namespace {
constexpr float kPi = 3.14159265358979323846f;
constexpr float kDegToRad = kPi / 180.0f;

std::array<std::uint8_t, 4> colorForFlag(avs::effects::GateFlag flag) {
  using avs::effects::GateFlag;
  switch (flag) {
    case GateFlag::Beat:
      return {200, 40, 40, 255};
    case GateFlag::Hold:
      return {40, 160, 40, 255};
    case GateFlag::Sticky:
      return {220, 220, 40, 255};
    case GateFlag::Off:
    default:
      return {24, 24, 24, 255};
  }
}

std::array<float, 2> clampPoint(std::array<float, 2> point, int width, int height) {
  point[0] = std::clamp(point[0], 0.0f, static_cast<float>(std::max(0, width - 1)));
  point[1] = std::clamp(point[1], 0.0f, static_cast<float>(std::max(0, height - 1)));
  return point;
}

std::array<float, 2> anchorFromToken(const std::string& token) {
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

}  // namespace

namespace avs::effects {

TransformAffine::TransformAffine() {
  gateConfig_.options.holdFrames = 2;
  gateConfig_.gate.configure(gateConfig_.options);
}

static std::array<std::uint8_t, 4> unpackColor(int value, std::array<std::uint8_t, 4> fallback) {
  if (value < 0) {
    return fallback;
  }
  const std::uint32_t packed = static_cast<std::uint32_t>(value);
  return {static_cast<std::uint8_t>((packed >> 16) & 0xFFu),
          static_cast<std::uint8_t>((packed >> 8) & 0xFFu),
          static_cast<std::uint8_t>(packed & 0xFFu), fallback[3]};
}

void TransformAffine::setParams(const avs::core::ParamBlock& params) {
  const auto originalAnchor = anchorNorm_;
  if (params.contains("anchor")) {
    anchorNorm_ = anchorFromToken(params.getString("anchor", ""));
  }
  if (params.getBool("center", false)) {
    anchorNorm_ = {0.5f, 0.5f};
  }
  if (params.contains("anchor_x") || params.contains("anchor_y")) {
    anchorNorm_[0] = std::clamp(params.getFloat("anchor_x", anchorNorm_[0]), 0.0f, 1.0f);
    anchorNorm_[1] = std::clamp(params.getFloat("anchor_y", anchorNorm_[1]), 0.0f, 1.0f);
  }
  if (params.getBool("reset_anchor", false)) {
    anchorNorm_ = originalAnchor;
  }

  baseAngleDeg_ = params.getFloat("angle", baseAngleDeg_);
  if (params.contains("rotate")) {
    if (params.getBool("rotate", false) && !params.contains("rotate_speed")) {
      rotateSpeedDeg_ = 90.0f;
    } else {
      rotateSpeedDeg_ = params.getFloat("rotate", rotateSpeedDeg_);
    }
  }
  rotateSpeedDeg_ = params.getFloat("rotate_speed", rotateSpeedDeg_);
  scale_ = params.getFloat("scale", scale_);
  doubleSize_ = params.getBool("2x", params.getBool("double", doubleSize_));
  testMode_ = params.getBool("test", params.getBool("debug", testMode_));
  randomPosition_ = params.getBool("randompos", randomPosition_);
  fiftyBlend_ = params.getBool("5050", fiftyBlend_) || params.getBool("fifty", fiftyBlend_);
  logRows_ = std::clamp(params.getInt("log_rows", logRows_), 1, 8);

  color_ = unpackColor(params.getInt("color", -1), color_);
  crossColor_ = unpackColor(params.getInt("cross_color", -1), crossColor_);

  random_.angleAmplitude = params.getFloat("random_angle", random_.angleAmplitude);
  random_.scaleAmplitude = params.getFloat("random_scale", random_.scaleAmplitude);
  random_.offsetAmplitude = params.getFloat("random_offset", random_.offsetAmplitude);

  GateOptions options = gateConfig_.options;
  options.enableOnBeat = params.getBool("onbeat", options.enableOnBeat);
  options.stickyToggle = params.getBool("stick", options.stickyToggle);
  options.onlySticky = params.getBool("onlysticky", options.onlySticky);
  options.holdFrames = std::max(1, params.getInt("hold", options.holdFrames));
  gateConfig_.options = options;
  gateConfig_.gate.configure(options);
  gateConfig_.gate.reset();
  history_.clear();
  historyLimit_ = 0;
  jitter_ = {0.0f, 0.0f};
  randomAngleOffset_ = 0.0f;
  randomScaleFactor_ = 1.0f;
}

void TransformAffine::updateRandom(avs::core::RenderContext& context, bool beatTriggered) {
  if (!beatTriggered) {
    return;
  }
  if (random_.angleAmplitude > 0.0f) {
    randomAngleOffset_ = context.rng.uniform(-random_.angleAmplitude, random_.angleAmplitude);
  } else {
    randomAngleOffset_ = 0.0f;
  }

  if (random_.scaleAmplitude > 0.0f) {
    randomScaleFactor_ = 1.0f + context.rng.uniform(-random_.scaleAmplitude, random_.scaleAmplitude);
  } else {
    randomScaleFactor_ = 1.0f;
  }

  if (randomPosition_ || random_.offsetAmplitude > 0.0f) {
    const float amplitude = (random_.offsetAmplitude > 0.0f) ? random_.offsetAmplitude : 0.25f;
    const float dx = context.rng.uniform(-amplitude, amplitude);
    const float dy = context.rng.uniform(-amplitude, amplitude);
    jitter_[0] = dx * static_cast<float>(context.width);
    jitter_[1] = dy * static_cast<float>(context.height);
  } else {
    jitter_ = {0.0f, 0.0f};
  }
}

void TransformAffine::drawTriangle(avs::core::RenderContext& context,
                                   const std::array<std::array<float, 2>, 3>& vertices,
                                   bool fiftyBlend) const {
  if (!context.framebuffer.data || context.width <= 0 || context.height <= 0) {
    return;
  }
  Triangle2D tri{vertices[0], vertices[1], vertices[2]};
  if (std::fabs(signedArea(tri)) < 1e-4f) {
    return;
  }

  float minX = std::min({vertices[0][0], vertices[1][0], vertices[2][0]});
  float maxX = std::max({vertices[0][0], vertices[1][0], vertices[2][0]});
  float minY = std::min({vertices[0][1], vertices[1][1], vertices[2][1]});
  float maxY = std::max({vertices[0][1], vertices[1][1], vertices[2][1]});

  const int x0 = std::max(0, static_cast<int>(std::floor(minX - 1.0f)));
  const int x1 = std::min(context.width - 1, static_cast<int>(std::ceil(maxX + 1.0f)));
  const int y0 = std::max(0, static_cast<int>(std::floor(minY - 1.0f)));
  const int y1 = std::min(context.height - 1, static_cast<int>(std::ceil(maxY + 1.0f)));

  for (int y = y0; y <= y1; ++y) {
    for (int x = x0; x <= x1; ++x) {
      const std::array<float, 2> p{static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f};
      if (!containsPoint(tri, p)) {
        continue;
      }
      const std::size_t offset =
          (static_cast<std::size_t>(y) * static_cast<std::size_t>(context.width) + static_cast<std::size_t>(x)) *
          4u;
      if (offset + 3 >= context.framebuffer.size) {
        continue;
      }
      std::uint8_t* dst = context.framebuffer.data + offset;
      if (fiftyBlend) {
        dst[0] = static_cast<std::uint8_t>((dst[0] + color_[0]) >> 1);
        dst[1] = static_cast<std::uint8_t>((dst[1] + color_[1]) >> 1);
        dst[2] = static_cast<std::uint8_t>((dst[2] + color_[2]) >> 1);
        dst[3] = 255;
      } else {
        dst[0] = color_[0];
        dst[1] = color_[1];
        dst[2] = color_[2];
        dst[3] = 255;
      }
    }
  }
}

void TransformAffine::drawCrosshair(avs::core::RenderContext& context, float x, float y) const {
  if (!context.framebuffer.data) {
    return;
  }
  const int ix = static_cast<int>(std::round(x));
  const int iy = static_cast<int>(std::round(y));
  constexpr int kRadius = 3;
  for (int dx = -kRadius; dx <= kRadius; ++dx) {
    const int px = ix + dx;
    if (px < 0 || px >= context.width || iy < 0 || iy >= context.height) continue;
    const std::size_t offset =
        (static_cast<std::size_t>(iy) * static_cast<std::size_t>(context.width) + static_cast<std::size_t>(px)) * 4u;
    if (offset + 3 >= context.framebuffer.size) continue;
    std::uint8_t* dst = context.framebuffer.data + offset;
    dst[0] = crossColor_[0];
    dst[1] = crossColor_[1];
    dst[2] = crossColor_[2];
    dst[3] = 255;
  }
  for (int dy = -kRadius; dy <= kRadius; ++dy) {
    const int py = iy + dy;
    if (py < 0 || py >= context.height || ix < 0 || ix >= context.width) continue;
    const std::size_t offset =
        (static_cast<std::size_t>(py) * static_cast<std::size_t>(context.width) + static_cast<std::size_t>(ix)) * 4u;
    if (offset + 3 >= context.framebuffer.size) continue;
    std::uint8_t* dst = context.framebuffer.data + offset;
    dst[0] = crossColor_[0];
    dst[1] = crossColor_[1];
    dst[2] = crossColor_[2];
    dst[3] = 255;
  }
}

void TransformAffine::drawGatingLog(avs::core::RenderContext& context) const {
  if (!context.framebuffer.data || history_.empty() || context.width <= 0 || context.height <= 0) {
    return;
  }
  const int rows = std::min(logRows_, context.height);
  const std::size_t width = static_cast<std::size_t>(context.width);
  const auto offColor = colorForFlag(GateFlag::Off);

  for (int row = 0; row < rows; ++row) {
    const int y = context.height - 1 - row;
    for (int x = 0; x < context.width; ++x) {
      const std::size_t offset =
          (static_cast<std::size_t>(y) * static_cast<std::size_t>(context.width) + static_cast<std::size_t>(x)) * 4u;
      if (offset + 3 >= context.framebuffer.size) continue;
      std::uint8_t* dst = context.framebuffer.data + offset;
      dst[0] = offColor[0];
      dst[1] = offColor[1];
      dst[2] = offColor[2];
      dst[3] = 255;
    }

    const std::size_t rowOffset = width * static_cast<std::size_t>(row);
    if (history_.size() <= rowOffset) {
      continue;
    }
    const std::size_t available = history_.size() - rowOffset;
    const std::size_t toDraw = std::min<std::size_t>(width, available);
    if (toDraw == 0) {
      continue;
    }
    const std::size_t startIndex = history_.size() - rowOffset - toDraw;
    const int startX = context.width - static_cast<int>(toDraw);
    for (int x = startX; x < context.width; ++x) {
      const std::size_t histIdx = startIndex + static_cast<std::size_t>(x - startX);
      if (histIdx >= history_.size()) {
        break;
      }
      const auto color = colorForFlag(history_[histIdx].flag);
      const std::size_t offset =
          (static_cast<std::size_t>(y) * static_cast<std::size_t>(context.width) + static_cast<std::size_t>(x)) * 4u;
      if (offset + 3 >= context.framebuffer.size) continue;
      std::uint8_t* dst = context.framebuffer.data + offset;
      dst[0] = color[0];
      dst[1] = color[1];
      dst[2] = color[2];
      dst[3] = 255;
    }
  }
}

bool TransformAffine::render(avs::core::RenderContext& context) {
  if (context.width <= 0 || context.height <= 0 || !context.framebuffer.data) {
    return true;
  }

  GateResult gate = gateConfig_.gate.step(context.audioBeat);
  updateRandom(context, gate.flag == GateFlag::Beat);

  const float widthF = static_cast<float>(context.width - 1);
  const float heightF = static_cast<float>(context.height - 1);
  auto anchorPx = clampPoint({anchorNorm_[0] * widthF, anchorNorm_[1] * heightF}, context.width, context.height);
  anchorPx[0] = std::clamp(anchorPx[0] + jitter_[0], 0.0f, widthF);
  anchorPx[1] = std::clamp(anchorPx[1] + jitter_[1], 0.0f, heightF);

  const std::size_t targetHistoryLimit =
      static_cast<std::size_t>(context.width) * static_cast<std::size_t>(logRows_);
  if (historyLimit_ != targetHistoryLimit) {
    historyLimit_ = std::max<std::size_t>(1u, targetHistoryLimit);
    if (history_.size() > historyLimit_) {
      const std::size_t trim = history_.size() - historyLimit_;
      history_.erase(history_.begin(), history_.begin() + static_cast<std::ptrdiff_t>(trim));
    }
  }
  history_.push_back({gate.flag});
  while (history_.size() > historyLimit_) {
    history_.pop_front();
  }

  if (gate.render) {
    const float baseScale = std::min(widthF, heightF) * 0.25f * scale_ * (doubleSize_ ? 2.0f : 1.0f) *
                            std::max(0.1f, randomScaleFactor_);
    const float angle = (baseAngleDeg_ + rotateSpeedDeg_ * static_cast<float>(context.frameIndex) + randomAngleOffset_) *
                        kDegToRad;

    Affine2D transform = Affine2D::translation(anchorPx[0], anchorPx[1]);
    transform = transform * Affine2D::rotation(angle);
    transform = transform * Affine2D::scale(baseScale, baseScale);

    std::array<std::array<float, 2>, 3> base{{{0.0f, -1.0f}, {0.8660254f, 0.5f}, {-0.8660254f, 0.5f}}};
    std::array<std::array<float, 2>, 3> vertices{
        transform.apply(base[0]), transform.apply(base[1]), transform.apply(base[2]),
    };
    drawTriangle(context, vertices, fiftyBlend_);
  }

  drawGatingLog(context);
  if (testMode_) {
    drawCrosshair(context, anchorPx[0], anchorPx[1]);
  }
  return true;
}

}  // namespace avs::effects
