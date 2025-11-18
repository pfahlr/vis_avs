#include <avs/core/IFramebuffer.hpp>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

namespace avs {
namespace core {

/**
 * @brief OpenGL framebuffer implementation using FBO and texture.
 *
 * Provides GPU-accelerated rendering with optional CPU readback.
 * Uses OpenGL 3.3 core profile features (FBO, texture 2D).
 */
class OpenGLFramebuffer : public IFramebuffer {
public:
  OpenGLFramebuffer(int width, int height) : width_(width), height_(height) {
    if (width <= 0 || height <= 0) {
      throw std::invalid_argument("OpenGLFramebuffer: invalid dimensions");
    }

    // Create texture for color attachment
    glGenTextures(1, &texture_);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Create framebuffer object
    glGenFramebuffers(1, &fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_, 0);

    // Verify framebuffer completeness
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
      glDeleteFramebuffers(1, &fbo_);
      glDeleteTextures(1, &texture_);
      throw std::runtime_error("OpenGLFramebuffer: FBO not complete, status=" +
                               std::to_string(status));
    }

    // Unbind
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Allocate CPU buffer for data() access
    cpuBuffer_.resize(width * height * 4, 0);
    cpuBufferDirty_ = true;
  }

  ~OpenGLFramebuffer() override {
    if (fbo_ != 0) {
      glDeleteFramebuffers(1, &fbo_);
    }
    if (texture_ != 0) {
      glDeleteTextures(1, &texture_);
    }
  }

  // Disable copy, enable move
  OpenGLFramebuffer(const OpenGLFramebuffer&) = delete;
  OpenGLFramebuffer& operator=(const OpenGLFramebuffer&) = delete;
  OpenGLFramebuffer(OpenGLFramebuffer&&) = default;
  OpenGLFramebuffer& operator=(OpenGLFramebuffer&&) = default;

  int width() const override { return width_; }

  int height() const override { return height_; }

  uint8_t* data() override {
    // Download GPU data to CPU buffer if needed
    if (cpuBufferDirty_) {
      download(cpuBuffer_.data(), cpuBuffer_.size());
      cpuBufferDirty_ = false;
    }
    return cpuBuffer_.data();
  }

  const uint8_t* data() const override {
    // Download GPU data to CPU buffer if needed
    if (cpuBufferDirty_) {
      download(const_cast<uint8_t*>(cpuBuffer_.data()), cpuBuffer_.size());
      cpuBufferDirty_ = false;
    }
    return cpuBuffer_.data();
  }

  size_t sizeBytes() const override { return width_ * height_ * 4; }

  void upload(const uint8_t* sourceData, size_t numBytes) override {
    if (!sourceData) {
      throw std::invalid_argument("OpenGLFramebuffer::upload: null source data");
    }
    const size_t expected = sizeBytes();
    if (numBytes != expected) {
      throw std::invalid_argument("OpenGLFramebuffer::upload: size mismatch (expected " +
                                  std::to_string(expected) + ", got " + std::to_string(numBytes) +
                                  ")");
    }

    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_, GL_RGBA, GL_UNSIGNED_BYTE,
                    sourceData);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Copy to CPU buffer and mark clean
    std::copy(sourceData, sourceData + numBytes, cpuBuffer_.begin());
    cpuBufferDirty_ = false;
  }

  void download(uint8_t* destData, size_t numBytes) const override {
    if (!destData) {
      throw std::invalid_argument("OpenGLFramebuffer::download: null destination data");
    }
    const size_t expected = sizeBytes();
    if (numBytes != expected) {
      throw std::invalid_argument("OpenGLFramebuffer::download: size mismatch (expected " +
                                  std::to_string(expected) + ", got " + std::to_string(numBytes) +
                                  ")");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glReadPixels(0, 0, width_, height_, GL_RGBA, GL_UNSIGNED_BYTE, destData);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  void present() override {
    // OpenGL framebuffer doesn't directly present to screen
    // This is handled by the window system (e.g., SDL swap buffers)
    // For FBO-based rendering, this is a no-op
    // The texture can be blitted to the screen by the window system
  }

  void clear(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0, uint8_t a = 255) override {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glClearColor(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Mark CPU buffer dirty since GPU was modified
    cpuBufferDirty_ = true;
  }

  void resize(int newWidth, int newHeight) override {
    if (newWidth <= 0 || newHeight <= 0) {
      throw std::invalid_argument("OpenGLFramebuffer::resize: invalid dimensions");
    }

    if (newWidth == width_ && newHeight == height_) {
      return;  // No change
    }

    width_ = newWidth;
    height_ = newHeight;

    // Recreate texture
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width_, height_, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Resize CPU buffer
    cpuBuffer_.resize(width_ * height_ * 4, 0);
    cpuBufferDirty_ = true;
  }

  bool supportsDirectAccess() const override {
    // Direct access is available but requires download from GPU
    return true;
  }

  const char* backendName() const override { return "OpenGL"; }

  /**
   * @brief Get the OpenGL texture ID for this framebuffer.
   * @return GLuint texture ID
   *
   * This allows external code to render to the texture or use it in shaders.
   */
  GLuint textureId() const { return texture_; }

  /**
   * @brief Get the OpenGL FBO ID for this framebuffer.
   * @return GLuint FBO ID
   *
   * This allows external code to bind the FBO for rendering.
   */
  GLuint fboId() const { return fbo_; }

  /**
   * @brief Bind this framebuffer for rendering.
   *
   * All subsequent OpenGL draw calls will render to this framebuffer.
   */
  void bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glViewport(0, 0, width_, height_);
    cpuBufferDirty_ = true;  // GPU will be modified
  }

  /**
   * @brief Unbind this framebuffer (restore default framebuffer).
   */
  void unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

private:
  int width_;
  int height_;
  GLuint texture_ = 0;
  GLuint fbo_ = 0;
  mutable std::vector<uint8_t> cpuBuffer_;
  mutable bool cpuBufferDirty_ = true;  // True if GPU has changes not in CPU buffer
};

std::unique_ptr<IFramebuffer> createOpenGLFramebuffer(int width, int height) {
  return std::make_unique<OpenGLFramebuffer>(width, height);
}

}  // namespace core
}  // namespace avs
