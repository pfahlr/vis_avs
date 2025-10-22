#include "effects/trans/effect_multi_delay.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <charconv>
#include <system_error>

namespace avs::effects::trans {
namespace {

constexpr std::size_t kBufferCount = 6u;
constexpr std::size_t kMaxHistoryFrames = 400u;

std::string trimCopy(std::string_view text) {
  std::size_t start = 0;
  while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start]))) {
    ++start;
  }
  std::size_t end = text.size();
  while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
    --end;
  }
  return std::string(text.substr(start, end - start));
}

std::vector<std::string> makeBeatKeys(std::size_t index) {
  const std::string idx = std::to_string(index);
  return {
      std::string("usebeat") + idx,
      std::string("usebeat_") + idx,
      std::string("usebeats") + idx,
      std::string("usebeats_") + idx,
      std::string("useBeat") + idx,
      std::string("useBeat_") + idx,
      std::string("useBeats") + idx,
      std::string("useBeats_") + idx,
      std::string("use_beat") + idx,
      std::string("use_beats") + idx,
      std::string("buffer") + idx + "_usebeat",
      std::string("buffer") + idx + "_useBeat",
      std::string("buffer") + idx + "_use_beats",
      std::string("buffer") + idx + "_beats",
      std::string("buffer") + idx + "_beat",
  };
}

std::vector<std::string> makeDelayKeys(std::size_t index) {
  const std::string idx = std::to_string(index);
  return {
      std::string("delay") + idx,
      std::string("delay_") + idx,
      std::string("delayFrames") + idx,
      std::string("delayFrames_") + idx,
      std::string("delay_frames") + idx,
      std::string("delay_frames_") + idx,
      std::string("delay_frame") + idx,
      std::string("frames") + idx,
      std::string("frames_") + idx,
      std::string("frame") + idx,
      std::string("buffer") + idx + "_delay",
      std::string("buffer") + idx + "_frames",
      std::string("buffer") + idx + "_delay_frames",
      std::string("buffer") + idx + "_history",
  };
}

std::optional<bool> parseBoolLike(const avs::core::ParamBlock& params, const std::string& key) {
  if (!params.contains(key)) {
    return std::nullopt;
  }

  const std::string sentinel = "__avs_multi_delay_sentinel__";
  const std::string rawValue = params.getString(key, sentinel);
  if (rawValue != sentinel) {
    std::string trimmed = trimCopy(rawValue);
    if (!trimmed.empty()) {
      std::string lowered = trimmed;
      std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
      });
      if (lowered == "true" || lowered == "on" || lowered == "yes") {
        return true;
      }
      if (lowered == "false" || lowered == "off" || lowered == "no") {
        return false;
      }
      int parsed = 0;
      const char* begin = trimmed.c_str();
      const char* end = begin + trimmed.size();
      int base = 10;
      if (trimmed.size() > 2 && trimmed[0] == '0' && (trimmed[1] == 'x' || trimmed[1] == 'X')) {
        begin += 2;
        base = 16;
      } else if (!trimmed.empty() && trimmed.front() == '#') {
        ++begin;
        base = 16;
      }
      const auto result = std::from_chars(begin, end, parsed, base);
      if (result.ec == std::errc() && result.ptr == end) {
        return parsed != 0;
      }
    }
  }

  const int sentinelInt = std::numeric_limits<int>::min();
  const int intValue = params.getInt(key, sentinelInt);
  if (intValue != sentinelInt) {
    return intValue != 0;
  }

  return params.getBool(key, false);
}

std::optional<int> parseIntLike(const avs::core::ParamBlock& params, const std::string& key) {
  if (!params.contains(key)) {
    return std::nullopt;
  }

  const std::string sentinel = "__avs_multi_delay_sentinel__";
  const std::string rawValue = params.getString(key, sentinel);
  if (rawValue != sentinel) {
    std::string trimmed = trimCopy(rawValue);
    if (!trimmed.empty()) {
      int base = 10;
      const char* begin = trimmed.c_str();
      const char* end = begin + trimmed.size();
      if (trimmed.size() > 2 && trimmed[0] == '0' && (trimmed[1] == 'x' || trimmed[1] == 'X')) {
        begin += 2;
        base = 16;
      } else if (!trimmed.empty() && trimmed.front() == '#') {
        ++begin;
        base = 16;
      }
      int parsed = 0;
      const auto result = std::from_chars(begin, end, parsed, base);
      if (result.ec == std::errc() && result.ptr == end) {
        return parsed;
      }

      std::string lowered = trimmed;
      std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
      });
      if (lowered == "true" || lowered == "on" || lowered == "yes") {
        return 1;
      }
      if (lowered == "false" || lowered == "off" || lowered == "no") {
        return 0;
      }
    }
  }

  const int sentinelInt = std::numeric_limits<int>::min();
  const int value = params.getInt(key, sentinelInt);
  if (value != sentinelInt) {
    return value;
  }

  return params.getBool(key, false) ? 1 : 0;
}

struct BufferConfig {
  bool useBeat = false;
  int delayFrames = 0;
};

struct BufferRuntime {
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
    bool dirty = false;

    const auto globalBeat =
        extractBool(params, {"usebeat", "usebeats", "use_beats", "beats", "UseBeat", "UseBeats"});
    const auto globalDelay =
        extractInt(params, {"delay", "delay_frames", "frames", "Delay", "DelayFrames"});

    for (std::size_t i = 0; i < kBufferCount; ++i) {
      const auto beatValue = extractIndexedBool(params, i);
      const auto delayValue = extractIndexedInt(params, i);

      bool targetBeat = configs_[i].useBeat;
      int targetDelay = configs_[i].delayFrames;

      if (beatValue.has_value()) {
        targetBeat = *beatValue;
      } else if (globalBeat.has_value()) {
        targetBeat = *globalBeat;
      }

      if (delayValue.has_value()) {
        targetDelay = *delayValue;
      } else if (globalDelay.has_value()) {
        targetDelay = *globalDelay;
      }

      targetDelay = std::clamp(targetDelay, 0, static_cast<int>(kMaxHistoryFrames));

      if (targetBeat != configs_[i].useBeat || targetDelay != configs_[i].delayFrames) {
        configs_[i].useBeat = targetBeat;
        configs_[i].delayFrames = targetDelay;
        dirty = true;
      }
    }

    if (dirty) {
      configDirty_ = true;
    }
  }

  void beginFrame(const avs::core::RenderContext& context) {
    const std::size_t frameStride = computeFrameStride(context);
    const bool resolutionChanged = context.width != lastWidth_ || context.height != lastHeight_;
    const bool firstFrame = !haveFrame_;
    const bool newFrame = firstFrame || context.frameIndex != lastFrameIndex_;

    if (newFrame) {
      if (!firstFrame) {
        advanceFrame();
      }
      haveFrame_ = true;
      lastFrameIndex_ = context.frameIndex;
      lastWidth_ = context.width;
      lastHeight_ = context.height;
      lastFrameStride_ = frameStride;
      updateBeatCounters(context.audioBeat);
      reconfigureBuffers(frameStride, resolutionChanged || configDirty_);
      configDirty_ = false;
      return;
    }

    if (resolutionChanged || frameStride != lastFrameStride_) {
      lastWidth_ = context.width;
      lastHeight_ = context.height;
      lastFrameStride_ = frameStride;
      reconfigureBuffers(frameStride, true);
      configDirty_ = false;
      return;
    }

    if (configDirty_) {
      reconfigureBuffers(lastFrameStride_, true);
      configDirty_ = false;
    }
  }

  void storeFrame(int bufferIndex, const avs::core::RenderContext& context) {
    if (!validBufferIndex(bufferIndex)) {
      return;
    }
    BufferRuntime& buffer = buffers_[static_cast<std::size_t>(bufferIndex)];
    if (buffer.frameCount <= 1 || buffer.frameStride == 0 || buffer.storage.empty()) {
      return;
    }
    if (!context.framebuffer.data || context.framebuffer.size < buffer.frameStride) {
      return;
    }
    std::uint8_t* destination = buffer.storage.data() + buffer.writeIndex * buffer.frameStride;
    std::memcpy(destination, context.framebuffer.data, buffer.frameStride);
  }

  void fetchFrame(int bufferIndex, avs::core::RenderContext& context) const {
    if (!validBufferIndex(bufferIndex)) {
      return;
    }
    const BufferRuntime& buffer = buffers_[static_cast<std::size_t>(bufferIndex)];
    if (buffer.frameCount <= 1 || buffer.frameStride == 0 || buffer.storage.empty()) {
      return;
    }
    if (!context.framebuffer.data || context.framebuffer.size < buffer.frameStride) {
      return;
    }
    const std::uint8_t* source = buffer.storage.data() + buffer.readIndex * buffer.frameStride;
    std::memcpy(context.framebuffer.data, source, buffer.frameStride);
  }

 private:
  static bool validBufferIndex(int index) {
    return index >= 0 && index < static_cast<int>(kBufferCount);
  }

  static std::size_t computeFrameStride(const avs::core::RenderContext& context) {
    if (context.width <= 0 || context.height <= 0) {
      return 0;
    }
    return static_cast<std::size_t>(context.width) * static_cast<std::size_t>(context.height) * 4u;
  }

  static std::optional<bool> extractBool(const avs::core::ParamBlock& params,
                                         const std::initializer_list<const char*>& keys) {
    for (const char* key : keys) {
      if (auto value = parseBoolLike(params, key)) {
        return value;
      }
    }
    return std::nullopt;
  }

  static std::optional<int> extractInt(const avs::core::ParamBlock& params,
                                       const std::initializer_list<const char*>& keys) {
    for (const char* key : keys) {
      if (auto value = parseIntLike(params, key)) {
        return value;
      }
    }
    return std::nullopt;
  }

  static std::optional<bool> extractIndexedBool(const avs::core::ParamBlock& params, std::size_t index) {
    const auto candidates = makeBeatKeys(index);
    for (const auto& key : candidates) {
      if (auto value = parseBoolLike(params, key)) {
        return value;
      }
    }
    return std::nullopt;
  }

  static std::optional<int> extractIndexedInt(const avs::core::ParamBlock& params, std::size_t index) {
    const auto candidates = makeDelayKeys(index);
    for (const auto& key : candidates) {
      if (auto value = parseIntLike(params, key)) {
        return value;
      }
    }
    return std::nullopt;
  }

  void reset() {
    for (auto& buffer : buffers_) {
      buffer.storage.clear();
      buffer.storage.shrink_to_fit();
      buffer.frameStride = 0;
      buffer.frameCount = 0;
      buffer.readIndex = 0;
      buffer.writeIndex = 0;
    }
    for (auto& cfg : configs_) {
      cfg.useBeat = false;
      cfg.delayFrames = 0;
    }
    haveFrame_ = false;
    lastFrameIndex_ = std::numeric_limits<std::uint64_t>::max();
    lastFrameStride_ = 0;
    lastWidth_ = 0;
    lastHeight_ = 0;
    framesSinceBeat_ = 0;
    framesPerBeat_ = 0;
    configDirty_ = false;
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

  void reconfigureBuffers(std::size_t frameStride, bool forceRecreate) {
    for (std::size_t i = 0; i < kBufferCount; ++i) {
      const BufferConfig& cfg = configs_[i];
      const std::size_t delay = cfg.useBeat ? framesPerBeat_ : static_cast<std::size_t>(cfg.delayFrames);
      const std::size_t clampedDelay = std::min<std::size_t>(delay, kMaxHistoryFrames);
      const std::size_t frameCount = clampedDelay + 1u;
      configureBuffer(i, frameStride, frameCount, forceRecreate);
    }
  }

  void configureBuffer(std::size_t index, std::size_t frameStride, std::size_t frameCount, bool forceRecreate) {
    BufferRuntime& buffer = buffers_[index];

    if (frameStride == 0 || frameCount <= 1u) {
      buffer.storage.clear();
      buffer.frameStride = frameStride;
      buffer.frameCount = frameCount;
      buffer.readIndex = 0;
      buffer.writeIndex = frameCount > 0 ? frameCount - 1u : 0u;
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
    const bool sizeMismatch = buffer.storage.size() != requiredSize;
    const bool strideMismatch = buffer.frameStride != frameStride;
    const bool countMismatch = buffer.frameCount != frameCount;

    if (!forceRecreate && !sizeMismatch && !strideMismatch && !countMismatch) {
      if (buffer.readIndex >= frameCount) {
        buffer.readIndex %= frameCount;
      }
      if (buffer.writeIndex >= frameCount) {
        buffer.writeIndex %= frameCount;
      }
      return;
    }

    std::vector<std::uint8_t> newStorage(requiredSize, 0);
    if (!forceRecreate && !buffer.storage.empty() && buffer.frameCount > 0 && buffer.frameStride > 0) {
      const std::size_t framesToCopy = std::min(buffer.frameCount, frameCount);
      const std::size_t bytesToCopy = std::min(buffer.frameStride, frameStride);
      for (std::size_t i = 0; i < framesToCopy; ++i) {
        const std::size_t srcIndex = (buffer.readIndex + i) % buffer.frameCount;
        std::uint8_t* dst = newStorage.data() + i * frameStride;
        const std::uint8_t* src = buffer.storage.data() + srcIndex * buffer.frameStride;
        std::memcpy(dst, src, bytesToCopy);
        if (bytesToCopy < frameStride) {
          std::memset(dst + bytesToCopy, 0, frameStride - bytesToCopy);
        }
      }
    }

    buffer.storage = std::move(newStorage);
    buffer.frameStride = frameStride;
    buffer.frameCount = frameCount;
    buffer.readIndex = 0;
    buffer.writeIndex = frameCount - 1u;
  }

  void advanceFrame() {
    for (auto& buffer : buffers_) {
      if (buffer.frameCount <= 1u || buffer.storage.empty()) {
        continue;
      }
      buffer.readIndex = (buffer.readIndex + 1u) % buffer.frameCount;
      buffer.writeIndex = (buffer.writeIndex + 1u) % buffer.frameCount;
    }
  }

  std::array<BufferConfig, kBufferCount> configs_{};
  std::array<BufferRuntime, kBufferCount> buffers_{};
  std::size_t instanceCount_ = 0;
  bool haveFrame_ = false;
  bool configDirty_ = false;
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

  if (!context.framebuffer.data || context.width <= 0 || context.height <= 0) {
    return true;
  }

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

  const std::array<std::string, 4> modeKeys = {"mode", "operation", "op", "action"};
  for (const auto& key : modeKeys) {
    if (!params.contains(key)) {
      continue;
    }
    const std::string modeValue = params.getString(key, {});
    if (!modeValue.empty()) {
      if (setModeFromString(modeValue)) {
        goto modeHandled;
      }
    }
    setMode(params.getInt(key, static_cast<int>(mode_)));
    goto modeHandled;
  }
modeHandled:

  const std::array<std::string, 5> bufferKeys = {"buffer", "buffer_index", "slot", "activebuffer", "channel"};
  for (const auto& key : bufferKeys) {
    if (!params.contains(key)) {
      continue;
    }
    const std::string bufferValue = params.getString(key, {});
    if (!bufferValue.empty()) {
      if (setActiveBufferFromString(bufferValue)) {
        return;
      }
    }
    setActiveBuffer(params.getInt(key, activeBuffer_));
    return;
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

bool MultiDelay::setModeFromString(const std::string& value) {
  std::string lowered = value;
  std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  if (lowered == "store" || lowered == "write" || lowered == "put" || lowered == "save") {
    mode_ = Mode::Store;
    return true;
  }
  if (lowered == "fetch" || lowered == "read" || lowered == "get" || lowered == "load") {
    mode_ = Mode::Fetch;
    return true;
  }
  if (lowered == "passthrough" || lowered == "pass" || lowered == "none" || lowered == "idle") {
    mode_ = Mode::Passthrough;
    return true;
  }
  return false;
}

void MultiDelay::setActiveBuffer(int index) {
  if (index < 0) {
    activeBuffer_ = 0;
  } else if (index >= static_cast<int>(kBufferCount)) {
    activeBuffer_ = static_cast<int>(kBufferCount) - 1;
  } else {
    activeBuffer_ = index;
  }
}

bool MultiDelay::setActiveBufferFromString(const std::string& value) {
  if (value.empty()) {
    return false;
  }
  int parsed = 0;
  const char* begin = value.c_str();
  const char* end = begin + value.size();
  std::from_chars_result result = std::from_chars(begin, end, parsed);
  if (result.ec == std::errc() && result.ptr == end) {
    setActiveBuffer(parsed);
    return true;
  }
  if (value.size() == 1) {
    const char ch = value[0];
    if (ch >= 'A' && ch <= 'F') {
      setActiveBuffer(static_cast<int>(ch - 'A'));
      return true;
    }
    if (ch >= 'a' && ch <= 'f') {
      setActiveBuffer(static_cast<int>(ch - 'a'));
      return true;
    }
  }
  return false;
}

}  // namespace avs::effects::trans
