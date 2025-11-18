#include <avs/core/IFramebuffer.hpp>

// STB implementation is in stb_image_write_impl.cpp
#include "stb_image_write.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

namespace avs {
namespace core {

/**
 * @brief File-based framebuffer that writes frames to PNG files.
 *
 * Each present() call writes the current framebuffer to a PNG file.
 * Supports both single-file and frame sequence output patterns.
 */
class FileFramebuffer : public IFramebuffer {
public:
  FileFramebuffer(int width, int height, const std::string& outputPath)
      : width_(width), height_(height), outputPattern_(outputPath), frameCount_(0) {
    if (width <= 0 || height <= 0) {
      throw std::invalid_argument("FileFramebuffer: invalid dimensions");
    }
    if (outputPattern_.empty()) {
      throw std::invalid_argument("FileFramebuffer: empty output path");
    }

    pixels_.resize(width * height * 4, 0);

    // Create output directory if needed
    std::filesystem::path path(outputPattern_);
    if (path.has_parent_path()) {
      std::filesystem::path parentDir = path.parent_path();
      if (!parentDir.empty() && !std::filesystem::exists(parentDir)) {
        std::filesystem::create_directories(parentDir);
      }
    }
  }

  int width() const override { return width_; }

  int height() const override { return height_; }

  uint8_t* data() override { return pixels_.data(); }

  const uint8_t* data() const override { return pixels_.data(); }

  size_t sizeBytes() const override { return pixels_.size(); }

  void upload(const uint8_t* sourceData, size_t numBytes) override {
    if (!sourceData) {
      throw std::invalid_argument("FileFramebuffer::upload: null source data");
    }
    if (numBytes != sizeBytes()) {
      throw std::invalid_argument("FileFramebuffer::upload: size mismatch (expected " +
                                  std::to_string(sizeBytes()) + ", got " +
                                  std::to_string(numBytes) + ")");
    }
    std::copy(sourceData, sourceData + numBytes, pixels_.begin());
  }

  void download(uint8_t* destData, size_t numBytes) const override {
    if (!destData) {
      throw std::invalid_argument("FileFramebuffer::download: null destination data");
    }
    if (numBytes != sizeBytes()) {
      throw std::invalid_argument("FileFramebuffer::download: size mismatch (expected " +
                                  std::to_string(sizeBytes()) + ", got " +
                                  std::to_string(numBytes) + ")");
    }
    std::copy(pixels_.begin(), pixels_.end(), destData);
  }

  void present() override {
    // Generate filename from pattern
    std::string filename = generateFilename();

    // stb_image_write expects bottom-to-top layout, but our framebuffer is top-to-bottom
    // We need to flip vertically before writing
    std::vector<uint8_t> flipped(pixels_.size());
    const int rowBytes = width_ * 4;
    for (int y = 0; y < height_; ++y) {
      const uint8_t* srcRow = pixels_.data() + (y * rowBytes);
      uint8_t* dstRow = flipped.data() + ((height_ - 1 - y) * rowBytes);
      std::memcpy(dstRow, srcRow, rowBytes);
    }

    // Write PNG file
    int result = stbi_write_png(filename.c_str(), width_, height_, 4, flipped.data(), rowBytes);
    if (result == 0) {
      throw std::runtime_error("FileFramebuffer::present: failed to write PNG to " + filename);
    }

    frameCount_++;
  }

  void clear(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0, uint8_t a = 255) override {
    for (size_t i = 0; i < pixels_.size(); i += 4) {
      pixels_[i + 0] = r;
      pixels_[i + 1] = g;
      pixels_[i + 2] = b;
      pixels_[i + 3] = a;
    }
  }

  void resize(int newWidth, int newHeight) override {
    if (newWidth <= 0 || newHeight <= 0) {
      throw std::invalid_argument("FileFramebuffer::resize: invalid dimensions");
    }

    if (newWidth == width_ && newHeight == height_) {
      return;  // No change
    }

    width_ = newWidth;
    height_ = newHeight;
    pixels_.resize(width_ * height_ * 4, 0);
  }

  bool supportsDirectAccess() const override { return true; }

  const char* backendName() const override { return "File"; }

  /**
   * @brief Get the number of frames written so far.
   */
  int frameCount() const { return frameCount_; }

  /**
   * @brief Reset the frame counter to 0.
   */
  void resetFrameCount() { frameCount_ = 0; }

private:
  std::string generateFilename() const {
    // Check if pattern contains format specifiers (e.g., %05d)
    if (outputPattern_.find('%') != std::string::npos) {
      // Pattern-based naming (e.g., "frames/frame_%05d.png")
      char buffer[1024];
      std::snprintf(buffer, sizeof(buffer), outputPattern_.c_str(), frameCount_);
      return std::string(buffer);
    } else {
      // Single file mode - append frame number before extension
      std::filesystem::path path(outputPattern_);
      std::string stem = path.stem().string();
      std::string ext = path.extension().string();
      std::filesystem::path parent = path.parent_path();

      char buffer[64];
      std::snprintf(buffer, sizeof(buffer), "_%05d", frameCount_);

      std::filesystem::path result = parent / (stem + buffer + ext);
      return result.string();
    }
  }

  int width_;
  int height_;
  std::string outputPattern_;
  std::vector<uint8_t> pixels_;
  int frameCount_;
};

std::unique_ptr<IFramebuffer> createFileFramebuffer(int width, int height,
                                                     const std::string& outputPath) {
  return std::make_unique<FileFramebuffer>(width, height, outputPath);
}

}  // namespace core
}  // namespace avs
