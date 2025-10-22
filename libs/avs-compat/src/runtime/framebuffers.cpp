#include <avs/runtime/framebuffers.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

namespace avs::runtime {

namespace {
constexpr int kChannels = 4;

inline std::uint8_t saturatingAdd(std::uint8_t a, std::uint8_t b) {
  const int sum = static_cast<int>(a) + static_cast<int>(b);
  return static_cast<std::uint8_t>(std::min(sum, 255));
}

inline std::uint8_t average(std::uint8_t a, std::uint8_t b) {
  return static_cast<std::uint8_t>((static_cast<int>(a) + static_cast<int>(b)) / 2);
}

inline std::uint8_t blendDefault(std::uint8_t dst, std::uint8_t src) {
  // Treat "default blend" as 3/4 destination, 1/4 source to emulate a light fade.
  const int value = (static_cast<int>(dst) * 3 + static_cast<int>(src)) >> 2;
  return static_cast<std::uint8_t>(std::clamp(value, 0, 255));
}

struct Color {
  std::uint8_t a{255};
  std::uint8_t r{0};
  std::uint8_t g{0};
  std::uint8_t b{0};
};

Color unpackColor(std::uint32_t argb) {
  Color c;
  c.a = static_cast<std::uint8_t>((argb >> 24) & 0xFFu);
  c.r = static_cast<std::uint8_t>((argb >> 16) & 0xFFu);
  c.g = static_cast<std::uint8_t>((argb >> 8) & 0xFFu);
  c.b = static_cast<std::uint8_t>(argb & 0xFFu);
  return c;
}

}  // namespace

Framebuffers::Framebuffers(int width, int height) {
  resize(width, height);
}

void Framebuffers::resize(int width, int height) {
  width = std::max(width, 0);
  height = std::max(height, 0);
  allocate(buffers_[0], width, height);
  allocate(buffers_[1], width, height);
  std::fill(buffers_[0].pixels.begin(), buffers_[0].pixels.end(), 0);
  std::fill(buffers_[1].pixels.begin(), buffers_[1].pixels.end(), 0);
  for (auto& slot : slots_) {
    slot.clear();
  }
  slotValid_.fill(false);
  overlays_.fill({});
  current_ = 0;
  previous_ = 1;
  frameIndex_ = 0;
}

void Framebuffers::allocate(Frame& frame, int width, int height) {
  frame.width = width;
  frame.height = height;
  const std::size_t size = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * kChannels;
  frame.pixels.resize(size);
}

void Framebuffers::copyFrame(const Frame& src, Frame& dst) {
  if (dst.width != src.width || dst.height != src.height) {
    allocate(dst, src.width, src.height);
  }
  std::copy(src.pixels.begin(), src.pixels.end(), dst.pixels.begin());
}

void Framebuffers::beginFrame() {
  previous_ = current_;
  current_ ^= 1u;
  if (buffers_[current_].width != buffers_[previous_].width ||
      buffers_[current_].height != buffers_[previous_].height) {
    allocate(buffers_[current_], buffers_[previous_].width, buffers_[previous_].height);
  }
  std::copy(buffers_[previous_].pixels.begin(), buffers_[previous_].pixels.end(),
            buffers_[current_].pixels.begin());
}

void Framebuffers::finishFrame() {
  applyPersistentOverlays();
  ++frameIndex_;
}

FrameView Framebuffers::currentView() {
  Frame& frame = buffers_[current_];
  return FrameView{frame.pixels.data(), frame.width, frame.height, frame.width * kChannels};
}

FrameView Framebuffers::previousView() {
  Frame& frame = buffers_[previous_];
  return FrameView{frame.pixels.data(), frame.width, frame.height, frame.width * kChannels};
}

FrameView Framebuffers::currentView() const {
  const Frame& frame = buffers_[current_];
  return FrameView{const_cast<std::uint8_t*>(frame.pixels.data()), frame.width, frame.height,
                   frame.width * kChannels};
}

FrameView Framebuffers::previousView() const {
  const Frame& frame = buffers_[previous_];
  return FrameView{const_cast<std::uint8_t*>(frame.pixels.data()), frame.width, frame.height,
                   frame.width * kChannels};
}

std::size_t Framebuffers::slotIndex(BufferSlot slot) { return static_cast<std::size_t>(slot); }

int Framebuffers::wrapCoord(int value, int dimension) {
  if (dimension <= 0) return 0;
  int result = value % dimension;
  if (result < 0) result += dimension;
  return result;
}

void Framebuffers::clear(const ClearSettings& settings) {
  if (settings.firstFrameOnly && frameIndex_ > 0) {
    return;
  }
  FrameView view = currentView();
  if (!view.data) return;
  const Color color = unpackColor(settings.argb);
  const std::size_t pixelCount = static_cast<std::size_t>(view.width) * static_cast<std::size_t>(view.height);
  std::uint8_t* dst = view.data;
  for (std::size_t i = 0; i < pixelCount; ++i) {
    switch (settings.blend) {
      case ClearBlendMode::Replace:
        dst[0] = color.r;
        dst[1] = color.g;
        dst[2] = color.b;
        dst[3] = color.a;
        break;
      case ClearBlendMode::Additive:
        dst[0] = saturatingAdd(dst[0], color.r);
        dst[1] = saturatingAdd(dst[1], color.g);
        dst[2] = saturatingAdd(dst[2], color.b);
        dst[3] = saturatingAdd(dst[3], color.a);
        break;
      case ClearBlendMode::Average:
        dst[0] = average(dst[0], color.r);
        dst[1] = average(dst[1], color.g);
        dst[2] = average(dst[2], color.b);
        dst[3] = average(dst[3], color.a);
        break;
      case ClearBlendMode::DefaultBlend:
        dst[0] = blendDefault(dst[0], color.r);
        dst[1] = blendDefault(dst[1], color.g);
        dst[2] = blendDefault(dst[2], color.b);
        dst[3] = blendDefault(dst[3], color.a);
        break;
    }
    dst += kChannels;
  }
}

void Framebuffers::save(BufferSlot slot) {
  FrameView view = currentView();
  const std::size_t idx = slotIndex(slot);
  if (!view.data) {
    slots_[idx].clear();
    slotValid_[idx] = false;
    return;
  }
  auto& slotData = slots_[idx];
  slotData.assign(view.data, view.data + view.span().size());
  slotValid_[idx] = true;
}

void Framebuffers::restore(BufferSlot slot) {
  FrameView view = currentView();
  const std::size_t idx = slotIndex(slot);
  if (!view.data || !slotValid_[idx]) return;
  auto& slotData = slots_[idx];
  const std::size_t expected = view.span().size();
  if (slotData.size() != expected) {
    // Size mismatch invalidates the slot.
    slotValid_[idx] = false;
    return;
  }
  std::copy(slotData.begin(), slotData.end(), view.data);
}

void Framebuffers::wrap(const WrapSettings& settings) {
  FrameView src = previousView();
  FrameView dst = currentView();
  if (!src.data || !dst.data) return;
  for (int y = 0; y < dst.height; ++y) {
    const int sy = wrapCoord(y + settings.offsetY, src.height);
    const std::uint8_t* srcRow = src.data + sy * src.stride;
    std::uint8_t* dstRow = dst.data + y * dst.stride;
    for (int x = 0; x < dst.width; ++x) {
      const int sx = wrapCoord(x + settings.offsetX, src.width);
      const std::uint8_t* sp = srcRow + sx * kChannels;
      std::uint8_t* dp = dstRow + x * kChannels;
      std::copy(sp, sp + kChannels, dp);
    }
  }
}

void Framebuffers::slideIn(const SlideSettings& settings) {
  FrameView src = previousView();
  FrameView dst = currentView();
  if (!src.data || !dst.data) return;
  const int amount = std::max(0, settings.amount);
  for (int y = 0; y < dst.height; ++y) {
    std::uint8_t* dstRow = dst.data + y * dst.stride;
    for (int x = 0; x < dst.width; ++x) {
      std::uint8_t* dp = dstRow + x * kChannels;
      int sx = x;
      int sy = y;
      bool valid = true;
      switch (settings.direction) {
        case SlideDirection::Left:
          sx = x - amount;
          valid = sx >= 0;
          break;
        case SlideDirection::Right:
          sx = x + amount;
          valid = sx < src.width;
          break;
        case SlideDirection::Up:
          sy = y - amount;
          valid = sy >= 0;
          break;
        case SlideDirection::Down:
          sy = y + amount;
          valid = sy < src.height;
          break;
      }
      if (valid) {
        const std::uint8_t* sp = src.data + sy * src.stride + sx * kChannels;
        std::copy(sp, sp + kChannels, dp);
      } else {
        std::fill(dp, dp + kChannels, 0);
      }
    }
  }
}

void Framebuffers::slideOut(const SlideSettings& settings) {
  FrameView src = previousView();
  FrameView dst = currentView();
  if (!src.data || !dst.data) return;
  const int amount = std::max(0, settings.amount);
  for (int y = 0; y < dst.height; ++y) {
    std::uint8_t* dstRow = dst.data + y * dst.stride;
    for (int x = 0; x < dst.width; ++x) {
      std::uint8_t* dp = dstRow + x * kChannels;
      int sx = x;
      int sy = y;
      bool valid = true;
      switch (settings.direction) {
        case SlideDirection::Left:
          sx = x + amount;
          valid = sx < src.width;
          break;
        case SlideDirection::Right:
          sx = x - amount;
          valid = sx >= 0;
          break;
        case SlideDirection::Up:
          sy = y + amount;
          valid = sy < src.height;
          break;
        case SlideDirection::Down:
          sy = y - amount;
          valid = sy >= 0;
          break;
      }
      if (valid) {
        const std::uint8_t* sp = src.data + sy * src.stride + sx * kChannels;
        std::copy(sp, sp + kChannels, dp);
      } else {
        std::fill(dp, dp + kChannels, 0);
      }
    }
  }
}

void Framebuffers::transition(const TransitionSettings& settings) {
  FrameView src = previousView();
  FrameView dst = currentView();
  if (!src.data || !dst.data) return;
  const float t = std::clamp(settings.progress, 0.0f, 1.0f);
  const float invT = 1.0f - t;
  for (int y = 0; y < dst.height; ++y) {
    const std::uint8_t* srcRow = src.data + y * src.stride;
    std::uint8_t* dstRow = dst.data + y * dst.stride;
    for (int x = 0; x < dst.width; ++x) {
      const std::uint8_t* sp = srcRow + x * kChannels;
      std::uint8_t* dp = dstRow + x * kChannels;
      for (int c = 0; c < kChannels; ++c) {
        const float blended = static_cast<float>(sp[c]) * invT + static_cast<float>(dp[c]) * t;
        dp[c] = static_cast<std::uint8_t>(std::clamp(std::lround(blended), 0l, 255l));
      }
    }
  }
}

void Framebuffers::activateOverlay(OverlayChannel channel, const PersistSettings& settings) {
  const std::size_t idx = static_cast<std::size_t>(channel);
  if (idx >= overlays_.size()) return;
  auto& overlay = overlays_[idx];
  overlay.duration = std::max(0, settings.durationFrames);
  overlay.remaining = overlay.duration;
  overlay.color = settings.color;
}

void Framebuffers::applyPersistentOverlays() {
  FrameView view = currentView();
  if (!view.data) return;
  const std::array<int, 3> rows{0, 1, 2};
  for (std::size_t i = 0; i < overlays_.size(); ++i) {
    auto& overlay = overlays_[i];
    if (overlay.remaining <= 0 || overlay.duration <= 0) {
      continue;
    }
    const int row = rows[i];
    if (row >= view.height) {
      overlay.remaining = std::max(overlay.remaining - 1, 0);
      if (overlay.remaining == 0) overlay.duration = 0;
      continue;
    }
    const float strength = static_cast<float>(overlay.remaining) /
                           static_cast<float>(std::max(overlay.duration, 1));
    std::uint8_t* dstRow = view.data + row * view.stride;
    for (int x = 0; x < view.width; ++x) {
      std::uint8_t* px = dstRow + x * kChannels;
      for (int c = 0; c < 3; ++c) {
        const float dst = static_cast<float>(px[c]);
        const float src = static_cast<float>(overlay.color[c]);
        const float blended = dst * (1.0f - strength) + src * strength;
        px[c] = static_cast<std::uint8_t>(std::clamp(std::lround(blended), 0l, 255l));
      }
      px[3] = 255;
    }
    overlay.remaining = std::max(overlay.remaining - 1, 0);
    if (overlay.remaining == 0) {
      overlay.duration = 0;
    }
  }
}

void effect_clear(Framebuffers& fb, const ClearSettings& settings) { fb.clear(settings); }

void effect_save(Framebuffers& fb, BufferSlot slot) { fb.save(slot); }

void effect_restore(Framebuffers& fb, BufferSlot slot) { fb.restore(slot); }

void effect_wrap(Framebuffers& fb, const WrapSettings& settings) { fb.wrap(settings); }

void effect_in_slide(Framebuffers& fb, const SlideSettings& settings) { fb.slideIn(settings); }

void effect_out_slide(Framebuffers& fb, const SlideSettings& settings) { fb.slideOut(settings); }

void effect_transition(Framebuffers& fb, const TransitionSettings& settings) { fb.transition(settings); }

void effect_persist_title(Framebuffers& fb, const PersistSettings& settings) {
  fb.activateOverlay(OverlayChannel::Title, settings);
}

void effect_persist_text1(Framebuffers& fb, const PersistSettings& settings) {
  fb.activateOverlay(OverlayChannel::Text1, settings);
}

void effect_persist_text2(Framebuffers& fb, const PersistSettings& settings) {
  fb.activateOverlay(OverlayChannel::Text2, settings);
}

}  // namespace avs::runtime

