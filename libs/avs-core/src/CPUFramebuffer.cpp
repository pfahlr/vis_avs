#include "avs/core/IFramebuffer.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace avs {
namespace core {

namespace {

class CPUFramebuffer : public IFramebuffer {
public:
  CPUFramebuffer(int width, int height) : width_(width), height_(height) {
    if (width <= 0 || height <= 0) {
      throw std::invalid_argument("CPUFramebuffer: invalid dimensions");
    }
    pixels_.resize(width * height * 4, 0);
  }

  int width() const override { return width_; }
  int height() const override { return height_; }

  uint8_t* data() override { return pixels_.data(); }
  const uint8_t* data() const override { return pixels_.data(); }

  size_t sizeBytes() const override { return pixels_.size(); }

  void upload(const uint8_t* sourceData, size_t numBytes) override {
    if (!sourceData) {
      throw std::invalid_argument("CPUFramebuffer::upload: null source data");
    }
    if (numBytes != pixels_.size()) {
      throw std::invalid_argument("CPUFramebuffer::upload: size mismatch");
    }
    std::memcpy(pixels_.data(), sourceData, numBytes);
  }

  void download(uint8_t* destData, size_t numBytes) const override {
    if (!destData) {
      throw std::invalid_argument("CPUFramebuffer::download: null destination");
    }
    if (numBytes != pixels_.size()) {
      throw std::invalid_argument("CPUFramebuffer::download: size mismatch");
    }
    std::memcpy(destData, pixels_.data(), numBytes);
  }

  void present() override {
    // No-op for CPU framebuffer (no display)
  }

  void clear(uint8_t r, uint8_t g, uint8_t b, uint8_t a) override {
    // Pack as RGBA in memory (little-endian uint32)
    const uint32_t color = (static_cast<uint32_t>(a) << 24) |
                          (static_cast<uint32_t>(b) << 16) |
                          (static_cast<uint32_t>(g) << 8) |
                          static_cast<uint32_t>(r);
    uint32_t* pixels32 = reinterpret_cast<uint32_t*>(pixels_.data());
    const int totalPixels = width_ * height_;

    for (int i = 0; i < totalPixels; ++i) {
      pixels32[i] = color;
    }
  }

  void resize(int newWidth, int newHeight) override {
    if (newWidth <= 0 || newHeight <= 0) {
      throw std::invalid_argument("CPUFramebuffer::resize: invalid dimensions");
    }

    width_ = newWidth;
    height_ = newHeight;
    pixels_.resize(width_ * height_ * 4, 0);
  }

  bool supportsDirectAccess() const override { return true; }

  const char* backendName() const override { return "CPU"; }

private:
  int width_;
  int height_;
  std::vector<uint8_t> pixels_;
};

}  // namespace

std::unique_ptr<IFramebuffer> createCPUFramebuffer(int width, int height) {
  return std::make_unique<CPUFramebuffer>(width, height);
}

}  // namespace core
}  // namespace avs
