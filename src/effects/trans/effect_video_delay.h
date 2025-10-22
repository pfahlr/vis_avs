#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "avs/core/IEffect.hpp"

namespace avs::effects::trans {

class VideoDelay : public avs::core::IEffect {
 public:
  VideoDelay() = default;
  ~VideoDelay() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

  [[nodiscard]] int currentDelayFrames() const noexcept { return currentDelayFrames_; }

 private:
  void ensureBuffer(std::size_t frameSize, int requiredFrames);
  void ensureScratch(std::size_t frameSize);
  int computeDelayFrames(bool beatDetected);

  static constexpr int kMaxBeatMultiplier = 16;
  static constexpr int kMaxFrameDelay = 200;
  static constexpr int kMaxHistoryFrames = 400;

  bool enabled_ = true;
  bool useBeats_ = false;
  int baseDelay_ = 10;
  int currentDelayFrames_ = 10;
  int framesSinceBeat_ = 0;

  std::size_t frameSize_ = 0;
  std::size_t bufferFrameCount_ = 0;
  std::size_t headIndex_ = 0;
  std::size_t filledFrameCount_ = 0;
  std::vector<std::uint8_t> buffer_;
  std::vector<std::uint8_t> scratch_;
};

}  // namespace avs::effects::trans
