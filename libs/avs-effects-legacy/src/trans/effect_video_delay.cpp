#include <avs/effects/trans/effect_video_delay.h>

#include <algorithm>
#include <cstring>

namespace avs::effects::trans {

namespace {
constexpr int clampToRange(int value, int minValue, int maxValue) {
  return value < minValue ? minValue : (value > maxValue ? maxValue : value);
}
}  // namespace

void VideoDelay::setParams(const avs::core::ParamBlock& params) {
  bool newEnabled = enabled_;
  if (params.contains("enabled")) {
    newEnabled = params.getBool("enabled", newEnabled);
  }

  bool newUseBeats = useBeats_;
  if (params.contains("use_beats")) {
    newUseBeats = params.getBool("use_beats", newUseBeats);
  } else if (params.contains("useBeats")) {
    newUseBeats = params.getBool("useBeats", newUseBeats);
  } else if (params.contains("beats")) {
    newUseBeats = params.getBool("beats", newUseBeats);
  }

  int newDelay = baseDelay_;
  if (params.contains("delay")) {
    newDelay = params.getInt("delay", newDelay);
  } else if (params.contains("delay_frames")) {
    newDelay = params.getInt("delay_frames", newDelay);
  } else if (params.contains("frames")) {
    newDelay = params.getInt("frames", newDelay);
  }

  newDelay = std::max(newDelay, 0);
  const int maxDelay = newUseBeats ? kMaxBeatMultiplier : kMaxFrameDelay;
  newDelay = clampToRange(newDelay, 0, maxDelay);

  const bool modeChanged = newUseBeats != useBeats_;

  enabled_ = newEnabled;
  useBeats_ = newUseBeats;
  baseDelay_ = newDelay;

  if (useBeats_) {
    if (modeChanged) {
      framesSinceBeat_ = 0;
      currentDelayFrames_ = 0;
      filledFrameCount_ = 0;
      headIndex_ = 0;
    } else {
      currentDelayFrames_ = clampToRange(currentDelayFrames_, 0, kMaxHistoryFrames);
    }
  } else {
    framesSinceBeat_ = 0;
    currentDelayFrames_ = clampToRange(baseDelay_, 0, kMaxHistoryFrames);
  }
}

bool VideoDelay::render(avs::core::RenderContext& context) {
  const int requiredDelay = computeDelayFrames(context.audioBeat);

  if (!context.framebuffer.data || context.width <= 0 || context.height <= 0) {
    return true;
  }

  const std::size_t frameSize =
      static_cast<std::size_t>(context.width) * static_cast<std::size_t>(context.height) * 4u;
  if (context.framebuffer.size < frameSize) {
    return true;
  }

  if (!enabled_) {
    return true;
  }

  const int historyTarget = useBeats_ ? std::max(requiredDelay, framesSinceBeat_) : requiredDelay;

  ensureBuffer(frameSize, historyTarget);

  if (buffer_.empty() || bufferFrameCount_ == 0) {
    return true;
  }

  if (requiredDelay > 0) {
    ensureScratch(frameSize);
    std::uint8_t* slot = buffer_.data() + headIndex_ * frameSize;
    std::memcpy(scratch_.data(), context.framebuffer.data, frameSize);
    std::memcpy(context.framebuffer.data, slot, frameSize);
    std::memcpy(slot, scratch_.data(), frameSize);
    if (filledFrameCount_ < bufferFrameCount_) {
      ++filledFrameCount_;
    }
    headIndex_ = (headIndex_ + 1u) % bufferFrameCount_;
  } else if (useBeats_) {
    const std::size_t insertIndex = (headIndex_ + filledFrameCount_) % bufferFrameCount_;
    std::uint8_t* slot = buffer_.data() + insertIndex * frameSize;
    std::memcpy(slot, context.framebuffer.data, frameSize);
    if (filledFrameCount_ < bufferFrameCount_) {
      ++filledFrameCount_;
    } else {
      headIndex_ = (headIndex_ + 1u) % bufferFrameCount_;
    }
  }

  return true;
}

void VideoDelay::ensureBuffer(std::size_t frameSize, int requiredFrames) {
  const int clampedFrames = clampToRange(requiredFrames, 0, kMaxHistoryFrames);
  const std::size_t targetCount = static_cast<std::size_t>(clampedFrames);

  if (frameSize_ != frameSize) {
    frameSize_ = frameSize;
    buffer_.clear();
    bufferFrameCount_ = 0;
    headIndex_ = 0;
    filledFrameCount_ = 0;
  }

  if (targetCount == 0) {
    buffer_.clear();
    bufferFrameCount_ = 0;
    headIndex_ = 0;
    filledFrameCount_ = 0;
    return;
  }

  if (bufferFrameCount_ == targetCount && buffer_.size() == targetCount * frameSize_) {
    return;
  }

  std::vector<std::uint8_t> newBuffer(targetCount * frameSize_, 0);
  const std::size_t framesToCopy =
      std::min<std::size_t>(filledFrameCount_, std::min(bufferFrameCount_, targetCount));
  if (!buffer_.empty() && framesToCopy > 0 && bufferFrameCount_ > 0) {
    for (std::size_t i = 0; i < framesToCopy; ++i) {
      const std::size_t srcIndex = (headIndex_ + i) % bufferFrameCount_;
      std::memcpy(newBuffer.data() + i * frameSize_, buffer_.data() + srcIndex * frameSize_,
                  frameSize_);
    }
  }
  buffer_ = std::move(newBuffer);
  bufferFrameCount_ = targetCount;
  headIndex_ = 0;
  filledFrameCount_ = framesToCopy;
}

void VideoDelay::ensureScratch(std::size_t frameSize) {
  if (scratch_.size() < frameSize) {
    scratch_.resize(frameSize);
  }
}

int VideoDelay::computeDelayFrames(bool beatDetected) {
  if (useBeats_) {
    if (beatDetected) {
      const long long frames = static_cast<long long>(framesSinceBeat_);
      const long long scaled = frames * static_cast<long long>(baseDelay_);
      const int capped = static_cast<int>(std::min<long long>(scaled, kMaxHistoryFrames));
      currentDelayFrames_ = capped >= 0 ? capped : 0;
      framesSinceBeat_ = 0;
    }
    if (framesSinceBeat_ < kMaxHistoryFrames) {
      ++framesSinceBeat_;
      if (framesSinceBeat_ > kMaxHistoryFrames) {
        framesSinceBeat_ = kMaxHistoryFrames;
      }
    }
  } else {
    framesSinceBeat_ = 0;
    currentDelayFrames_ = clampToRange(baseDelay_, 0, kMaxHistoryFrames);
  }

  currentDelayFrames_ = clampToRange(currentDelayFrames_, 0, kMaxHistoryFrames);
  return currentDelayFrames_;
}

}  // namespace avs::effects::trans
