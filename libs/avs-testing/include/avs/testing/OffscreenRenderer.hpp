#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

namespace avs {
class Engine;
}

namespace avs::offscreen {

struct FrameView {
  const std::uint8_t* data = nullptr;
  std::size_t size = 0;
  int width = 0;
  int height = 0;
};

class OffscreenRenderer {
 public:
 explicit OffscreenRenderer(int width, int height);
  ~OffscreenRenderer();

  void loadPreset(const std::filesystem::path& presetPath);

  void setAudioBuffer(std::vector<float> samples, unsigned sampleRate, unsigned channels);

  FrameView render();

  [[nodiscard]] std::uint64_t frameIndex() const { return frameIndex_; }

  [[nodiscard]] int width() const { return width_; }
  [[nodiscard]] int height() const { return height_; }

 private:
  struct AudioTrack;

  FrameView currentFrame() const;

  int width_ = 0;
  int height_ = 0;
  double deltaSeconds_ = 1.0 / 60.0;
  std::unique_ptr<avs::Engine> engine_;
  std::unique_ptr<AudioTrack> audio_;
  std::filesystem::path presetPath_;
  bool presetLoaded_ = false;
  std::uint64_t frameIndex_ = 0;
};

}  // namespace avs::offscreen

