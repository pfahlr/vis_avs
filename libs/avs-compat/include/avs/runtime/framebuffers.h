#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace avs::runtime {

struct FrameView {
  std::uint8_t* data{nullptr};
  int width{0};
  int height{0};
  int stride{0};

  [[nodiscard]] std::span<std::uint8_t> span() const {
    if (!data || width <= 0 || height <= 0 || stride < width * 4) {
      return {};
    }
    const std::size_t bytes = static_cast<std::size_t>(height) * static_cast<std::size_t>(stride);
    return {data, bytes};
  }
};

enum class ClearBlendMode { Replace, Additive, Average, DefaultBlend };

enum class BufferSlot : std::size_t {
  A = 0,
  B,
  C,
  D,
  E,
  F,
  G,
  H,
};

enum class SlideDirection { Left, Right, Up, Down };

enum class OverlayChannel { Title = 0, Text1 = 1, Text2 = 2 };

struct ClearSettings {
  std::uint32_t argb{0xFF000000u};
  ClearBlendMode blend{ClearBlendMode::Replace};
  bool firstFrameOnly{false};
};

struct WrapSettings {
  int offsetX{0};
  int offsetY{0};
};

struct SlideSettings {
  SlideDirection direction{SlideDirection::Left};
  int amount{0};
};

struct TransitionSettings {
  float progress{1.0f};
};

struct PersistSettings {
  int durationFrames{0};
  std::array<std::uint8_t, 4> color{0, 0, 0, 255};
};

class Framebuffers {
 public:
  Framebuffers(int width, int height);

  void resize(int width, int height);

  void beginFrame();
  void finishFrame();

  [[nodiscard]] FrameView currentView();
  [[nodiscard]] FrameView previousView();
  [[nodiscard]] FrameView currentView() const;
  [[nodiscard]] FrameView previousView() const;

  [[nodiscard]] std::uint64_t frameIndex() const { return frameIndex_; }

  void clear(const ClearSettings& settings);
  void save(BufferSlot slot);
  void restore(BufferSlot slot);
  void wrap(const WrapSettings& settings);
  void slideIn(const SlideSettings& settings);
  void slideOut(const SlideSettings& settings);
  void transition(const TransitionSettings& settings);
  void activateOverlay(OverlayChannel channel, const PersistSettings& settings);

 private:
  struct Frame {
    int width{0};
    int height{0};
    std::vector<std::uint8_t> pixels{};
  };

  struct OverlayState {
    int remaining{0};
    int duration{0};
    std::array<std::uint8_t, 4> color{0, 0, 0, 255};
  };

  static std::size_t slotIndex(BufferSlot slot);
  static int wrapCoord(int value, int dimension);
  void allocate(Frame& frame, int width, int height);
  void copyFrame(const Frame& src, Frame& dst);
  void applyPersistentOverlays();

  std::array<Frame, 2> buffers_{};
  std::array<std::vector<std::uint8_t>, 8> slots_{};
  std::array<bool, 8> slotValid_{};
  std::array<OverlayState, 3> overlays_{};
  std::size_t current_{0};
  std::size_t previous_{1};
  std::uint64_t frameIndex_{0};
};

void effect_clear(Framebuffers& fb, const ClearSettings& settings);
void effect_save(Framebuffers& fb, BufferSlot slot);
void effect_restore(Framebuffers& fb, BufferSlot slot);
void effect_wrap(Framebuffers& fb, const WrapSettings& settings);
void effect_in_slide(Framebuffers& fb, const SlideSettings& settings);
void effect_out_slide(Framebuffers& fb, const SlideSettings& settings);
void effect_transition(Framebuffers& fb, const TransitionSettings& settings);
void effect_persist_title(Framebuffers& fb, const PersistSettings& settings);
void effect_persist_text1(Framebuffers& fb, const PersistSettings& settings);
void effect_persist_text2(Framebuffers& fb, const PersistSettings& settings);

}  // namespace avs::runtime

