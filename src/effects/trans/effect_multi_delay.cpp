#include "effects/trans/effect_multi_delay.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace avs::effects::trans {
namespace {

constexpr std::size_t kBufferCount = 6u;
// Matches the historical cap used by VideoDelay to keep allocations bounded.
constexpr std::size_t kMaxHistoryFrames = 400u;

struct BufferSettings {
  bool useBeat = false;
  int delayFrames = 0;
};

struct BufferState {
  std::vector<std::uint8_t> storage;
  std::size_t frameStride = 0;
  std::size_t frameCount = 0;
  std::size_t readIndex = 0;
  std::size_t writeIndex = 0;
};

class SharedState {
 public:
  static SharedState& instance() {
    static SharedState state;
    return state;
  }

  void addInstance() { ++instanceCount_; }

  void removeInstance() {
    if (instanceCount_ == 0) {
      return;
    }
    --instanceCount_;
    if (instanceCount_ == 0) {
      reset();
    }
  }

  void applyParams(const avs::core::ParamBlock& params) {
    for (std::size_t i = 0; i < kBufferCount; ++i) {
      const std::string index = std::to_string(i);
      const std::array<std::string, 4> beatKeys = {
          "usebeat" + index, "usebeats" + index, "use_beat" + index, "buffer" + index + "_usebeat"};
      const std::array<std::string, 3> delayKeys = {"delay" + index, "delay_" + index,
                                                    "buffer" + index + "_delay"};

      for (const auto& key : beatKeys) {
        if (!params.contains(key)) {
          continue;
        }
        const bool boolValue = params.getBool(key, settings_[i].useBeat);
        const int value = params.getInt(key, boolValue ? 1 : 0);
        settings_[i].useBeat = (value != 0);
        goto parsedBeat;
      }
      if (params.contains("usebeat")) {
        const bool boolValue = params.getBool("usebeat", settings_[i].useBeat);
        const int value = params.getInt("usebeat", boolValue ? 1 : 0);
        settings_[i].useBeat = (value != 0);
      } else if (params.contains("usebeats")) {
        const bool boolValue = params.getBool("usebeats", settings_[i].useBeat);
        const int value = params.getInt("usebeats", boolValue ? 1 : 0);
        settings_[i].useBeat = (value != 0);
      }
    parsedBeat:
      for (const auto& key : delayKeys) {
        if (!params.contains(key)) {
          continue;
        }
        const int value = params.getInt(key, settings_[i].delayFrames);
        settings_[i].delayFrames =
            std::clamp(value, 0, static_cast<int>(kMaxHistoryFrames));
        goto parsedDelay;
      }
      if (params.contains("delay")) {
        const int value = params.getInt("delay", settings_[i].delayFrames);
        settings_[i].delayFrames =
            std::clamp(value, 0, static_cast<int>(kMaxHistoryFrames));
      }
    parsedDelay:
      continue;
    }
  }

  void beginFrame(const avs::core::RenderContext& context) {
    const std::size_t frameStride = computeFrameStride(context);
    const bool resolutionChanged = (context.width != lastWidth_) || (context.height != lastHeight_);

    if (!hasFrame_) {
      hasFrame_ = true;
      lastFrameIndex_ = context.frameIndex;
      lastFrameStride_ = frameStride;
      lastWidth_ = context.width;
      lastHeight_ = context.height;
      framesSinceBeat_ = 0;
      framesPerBeat_ = 0;
      updateBeatCounters(context.audioBeat);
      configureBuffers(frameStride, true);
      return;
    }

    if (context.frameIndex != lastFrameIndex_) {
      finalizeFrame();
      lastFrameIndex_ = context.frameIndex;
      lastFrameStride_ = frameStride;
      lastWidth_ = context.width;
      lastHeight_ = context.height;
      updateBeatCounters(context.audioBeat);
      configureBuffers(frameStride, resolutionChanged);
      return;
    }

    if (frameStride != lastFrameStride_ || resolutionChanged) {
      lastFrameStride_ = frameStride;
      lastWidth_ = context.width;
      lastHeight_ = context.height;
      configureBuffers(frameStride, true);
    }
  }

  void storeFrame(int bufferIndex, const avs::core::RenderContext& context) {
    if (!isValidBuffer(bufferIndex)) {
      return;
    }
    BufferState& buffer = buffers_[static_cast<std::size_t>(bufferIndex)];
    if (buffer.frameCount <= 1 || buffer.frameStride == 0 || buffer.storage.empty()) {
      return;
    }
    if (!context.framebuffer.data || context.framebuffer.size < buffer.frameStride) {
      return;
    }
    const std::size_t offset = buffer.writeIndex * buffer.frameStride;
    std::copy_n(context.framebuffer.data, buffer.frameStride,
                buffer.storage.begin() + static_cast<std::ptrdiff_t>(offset));
  }

  void fetchFrame(int bufferIndex, avs::core::RenderContext& context) const {
    if (!isValidBuffer(bufferIndex)) {
      return;
    }
    const BufferState& buffer = buffers_[static_cast<std::size_t>(bufferIndex)];
    if (buffer.frameCount <= 1 || buffer.frameStride == 0 || buffer.storage.empty()) {
      return;
    }
    if (!context.framebuffer.data || context.framebuffer.size < buffer.frameStride) {
      return;
    }
    const std::size_t offset = buffer.readIndex * buffer.frameStride;
    std::copy_n(buffer.storage.begin() + static_cast<std::ptrdiff_t>(offset), buffer.frameStride,
                context.framebuffer.data);
  }

 private:
  void reset() {
    for (auto& buffer : buffers_) {
      buffer.storage.clear();
      buffer.storage.shrink_to_fit();
      buffer.frameStride = 0;
      buffer.frameCount = 0;
      buffer.readIndex = 0;
      buffer.writeIndex = 0;
    }
    for (auto& settings : settings_) {
      settings.useBeat = false;
      settings.delayFrames = 0;
    }
    framesSinceBeat_ = 0;
    framesPerBeat_ = 0;
    hasFrame_ = false;
    lastFrameIndex_ = std::numeric_limits<std::uint64_t>::max();
    lastFrameStride_ = 0;
    lastWidth_ = 0;
    lastHeight_ = 0;
  }

  static std::size_t computeFrameStride(const avs::core::RenderContext& context) {
    if (context.width <= 0 || context.height <= 0) {
      return 0;
    }
    return static_cast<std::size_t>(context.width) * static_cast<std::size_t>(context.height) * 4u;
  }

  void updateBeatCounters(bool beat) {
    if (beat) {
      framesPerBeat_ = std::min(framesSinceBeat_, kMaxHistoryFrames);
      framesSinceBeat_ = 0;
    }
    if (framesSinceBeat_ < kMaxHistoryFrames) {
      ++framesSinceBeat_;
    }
  }

  void configureBuffers(std::size_t frameStride, bool forceReset) {
    for (std::size_t i = 0; i < kBufferCount; ++i) {
      const auto& settings = settings_[i];
      const std::size_t baseDelay = settings.useBeat
                                        ? framesPerBeat_
                                        : static_cast<std::size_t>(settings.delayFrames);
      const std::size_t clampedDelay = std::min(baseDelay, kMaxHistoryFrames);
      const std::size_t frameCount = clampedDelay + 1u;
      configureBuffer(i, frameStride, frameCount, forceReset);
    }
  }

  void configureBuffer(std::size_t index, std::size_t frameStride, std::size_t frameCount,
                       bool forceReset) {
    BufferState& buffer = buffers_[index];
    if (frameStride == 0 || frameCount <= 1u) {
      if (buffer.frameCount != 0 || buffer.frameStride != 0) {
        buffer.storage.clear();
        buffer.frameStride = 0;
        buffer.frameCount = 0;
        buffer.readIndex = 0;
        buffer.writeIndex = 0;
      }
      return;
    }

    if (frameCount > std::numeric_limits<std::size_t>::max() / frameStride) {
      buffer.storage.clear();
      buffer.frameStride = 0;
      buffer.frameCount = 0;
      buffer.readIndex = 0;
      buffer.writeIndex = 0;
      return;
    }

    const std::size_t requiredSize = frameStride * frameCount;
    if (forceReset || buffer.frameStride != frameStride || buffer.frameCount != frameCount ||
        buffer.storage.size() != requiredSize) {
      buffer.storage.assign(requiredSize, 0);
      buffer.frameStride = frameStride;
      buffer.frameCount = frameCount;
      buffer.readIndex = 0;
      buffer.writeIndex = frameCount - 1u;
      return;
    }

    buffer.frameStride = frameStride;
    buffer.frameCount = frameCount;
    if (buffer.readIndex >= frameCount) {
      buffer.readIndex %= frameCount;
    }
    if (buffer.writeIndex >= frameCount) {
      buffer.writeIndex %= frameCount;
    }
  }

  void finalizeFrame() {
    for (auto& buffer : buffers_) {
      if (buffer.frameCount <= 1u || buffer.storage.empty()) {
        continue;
      }
      buffer.readIndex = (buffer.readIndex + 1u) % buffer.frameCount;
      buffer.writeIndex = (buffer.writeIndex + 1u) % buffer.frameCount;
    }
  }

  static bool isValidBuffer(int index) {
    return index >= 0 && index < static_cast<int>(kBufferCount);
  }

  std::array<BufferSettings, kBufferCount> settings_{};
  std::array<BufferState, kBufferCount> buffers_{};
  std::size_t instanceCount_ = 0;
  bool hasFrame_ = false;
  std::uint64_t lastFrameIndex_ = std::numeric_limits<std::uint64_t>::max();
  std::size_t lastFrameStride_ = 0;
  int lastWidth_ = 0;
  int lastHeight_ = 0;
  std::size_t framesSinceBeat_ = 0;
  std::size_t framesPerBeat_ = 0;
};

}  // namespace

MultiDelay::MultiDelay() { SharedState::instance().addInstance(); }

MultiDelay::~MultiDelay() { SharedState::instance().removeInstance(); }

bool MultiDelay::render(avs::core::RenderContext& context) {
  SharedState& state = SharedState::instance();
  state.beginFrame(context);

  switch (mode_) {
    case Mode::Store:
      state.storeFrame(activeBuffer_, context);
      break;
    case Mode::Fetch:
      state.fetchFrame(activeBuffer_, context);
      break;
    case Mode::Passthrough:
    default:
      break;
  }
  return true;
}

void MultiDelay::setParams(const avs::core::ParamBlock& params) {
  SharedState::instance().applyParams(params);

  if (params.contains("mode")) {
    setMode(params.getInt("mode", static_cast<int>(mode_)));
  } else if (params.contains("operation")) {
    setMode(params.getInt("operation", static_cast<int>(mode_)));
  }

  if (params.contains("buffer")) {
    setActiveBuffer(params.getInt("buffer", activeBuffer_));
  } else if (params.contains("buffer_index")) {
    setActiveBuffer(params.getInt("buffer_index", activeBuffer_));
  } else if (params.contains("activebuffer")) {
    setActiveBuffer(params.getInt("activebuffer", activeBuffer_));
  }
}

void MultiDelay::setMode(int value) {
  switch (value) {
    case 1:
      mode_ = Mode::Store;
      break;
    case 2:
      mode_ = Mode::Fetch;
      break;
    default:
      mode_ = Mode::Passthrough;
      break;
  }
}

void MultiDelay::setActiveBuffer(int index) {
  if (index < 0) {
    activeBuffer_ = 0;
    return;
  }
  if (index >= static_cast<int>(kBufferCount)) {
    activeBuffer_ = static_cast<int>(kBufferCount) - 1;
    return;
  }
  activeBuffer_ = index;
}

}  // namespace avs::effects::trans
