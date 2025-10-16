#include "gating.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace avs::core {
namespace {
struct Color {
  std::uint8_t r;
  std::uint8_t g;
  std::uint8_t b;
};

constexpr std::array<Color, 5> kFlagColors = {Color{90, 20, 20},   // 0 - inactive
                                               Color{20, 170, 60},  // 1 - active
                                               Color{245, 210, 40}, // 2 - triggered
                                               Color{30, 110, 210}, // 3 - rejected
                                               Color{140, 70, 190}}; // 4 - latched

constexpr std::size_t kHistoryLimit = 2048;

void setPixel(PixelBufferView& fb, int width, int height, int x, int y, const Color& c) {
  if (x < 0 || y < 0 || x >= width || y >= height) {
    return;
  }
  const std::size_t index = static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                             static_cast<std::size_t>(x);
  if (index * 4ull >= fb.size) {
    return;
  }
  std::uint8_t* pixel = fb.data + index * 4;
  pixel[0] = c.r;
  pixel[1] = c.g;
  pixel[2] = c.b;
  pixel[3] = 255u;
}

Color colorForFlag(std::uint8_t flag) {
  if (flag < kFlagColors.size()) {
    return kFlagColors[flag];
  }
  return Color{80, 80, 80};
}

}  // namespace

void BeatGatingEffect::setParams(const ParamBlock& params) {
  onBeat_ = params.getBool("onbeat", onBeat_);
  stick_ = params.getBool("stick", stick_);
  randomPosition_ = params.getBool("random_pos", randomPosition_);
  fiftyFifty_ = params.getBool("fifty_fifty", fiftyFifty_);
  onlySticky_ = params.getBool("only_sticky", onlySticky_);
  logHeight_ = std::clamp(params.getInt("log_height", logHeight_), 0, 64);
  if (!onBeat_) {
    active_ = true;
  } else {
    active_ = false;
    latched_ = false;
  }
}

bool BeatGatingEffect::render(RenderContext& context) {
  bool beatAllowed = !onlySticky_ || context.beat.sticky;
  bool beatRejected = onBeat_ && context.beat.isBeat && !beatAllowed;
  bool beatActive = onBeat_ && context.beat.isBeat && beatAllowed;
  bool triggered = false;

  if (onBeat_) {
    if (beatActive) {
      triggered = true;
      bool keepActive = !fiftyFifty_ || ((context.rng.nextUint32() & 1u) != 0u);
      if (keepActive) {
        active_ = true;
        if (randomPosition_) {
          offsetX_ = context.rng.uniform(-1.0f, 1.0f);
          offsetY_ = context.rng.uniform(-1.0f, 1.0f);
        } else {
          offsetX_ = 0.0f;
          offsetY_ = 0.0f;
        }
        latched_ = stick_ ? true : false;
      } else {
        if (stick_) {
          latched_ = false;
        }
        active_ = stick_ ? latched_ : false;
        offsetX_ = 0.0f;
        offsetY_ = 0.0f;
      }
    } else {
      if (stick_) {
        active_ = latched_;
      } else {
        active_ = false;
      }
      if (!active_) {
        offsetX_ = 0.0f;
        offsetY_ = 0.0f;
      }
    }
  } else {
    if (fiftyFifty_) {
      active_ = (context.rng.nextUint32() & 1u) != 0u;
    } else {
      active_ = true;
    }
    if (randomPosition_ && active_) {
      offsetX_ = context.rng.uniform(-1.0f, 1.0f);
      offsetY_ = context.rng.uniform(-1.0f, 1.0f);
    } else if (!active_) {
      offsetX_ = 0.0f;
      offsetY_ = 0.0f;
    }
    if (!stick_ || !active_) {
      latched_ = active_ && stick_;
    }
  }

  context.gating.active = active_;
  context.gating.latched = latched_;
  context.gating.triggered = triggered;
  context.gating.flag = stateToFlag(triggered, beatRejected);
  if (active_) {
    context.gating.randomOffsetX = offsetX_;
    context.gating.randomOffsetY = offsetY_;
  } else {
    context.gating.randomOffsetX = 0.0f;
    context.gating.randomOffsetY = 0.0f;
  }

  appendHistory(context.gating.flag);
  drawHistory(context);

  lastTriggered_ = triggered;
  lastFlag_ = context.gating.flag;

  return true;
}

void BeatGatingEffect::appendHistory(std::uint8_t flag) {
  history_.push_back(flag);
  if (history_.size() > kHistoryLimit) {
    history_.erase(history_.begin(), history_.begin() + (history_.size() - kHistoryLimit));
  }
}

void BeatGatingEffect::drawHistory(RenderContext& context) const {
  auto& fb = context.framebuffer;
  if (!fb.data || fb.size == 0 || context.width <= 0 || context.height <= 0) {
    return;
  }
  int rows = std::min(logHeight_, context.height);
  if (rows <= 0) {
    return;
  }
  int width = context.width;
  std::size_t available = std::min<std::size_t>(static_cast<std::size_t>(width), history_.size());
  if (available == 0) {
    return;
  }
  int padding = width - static_cast<int>(available);
  for (int y = 0; y < rows; ++y) {
    for (int x = 0; x < width; ++x) {
      if (x < padding) {
        setPixel(fb, width, context.height, x, y, colorForFlag(0));
        continue;
      }
      std::size_t historyIndex = history_.size() - available + static_cast<std::size_t>(x - padding);
      Color color = colorForFlag(history_[historyIndex]);
      setPixel(fb, width, context.height, x, y, color);
    }
  }
}

std::uint8_t BeatGatingEffect::stateToFlag(bool triggered, bool beatRejected) const {
  if (triggered) {
    return 2u;
  }
  if (beatRejected) {
    return 3u;
  }
  if (active_) {
    return latched_ ? 4u : 1u;
  }
  return 0u;
}

}  // namespace avs::core
