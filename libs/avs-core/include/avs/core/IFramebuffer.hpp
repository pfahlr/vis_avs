#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace avs {
namespace core {

/**
 * @brief Abstract framebuffer interface for rendering backends.
 *
 * Decouples effects from specific rendering implementations (OpenGL, Vulkan, CPU, etc.).
 * Effects write to the framebuffer through this interface without knowing the backend.
 */
class IFramebuffer {
public:
  virtual ~IFramebuffer() = default;

  /**
   * @brief Get framebuffer width in pixels.
   */
  virtual int width() const = 0;

  /**
   * @brief Get framebuffer height in pixels.
   */
  virtual int height() const = 0;

  /**
   * @brief Get direct access to pixel data (RGBA, 32-bit per pixel).
   * @return Pointer to pixel data, or nullptr if not supported by backend.
   *
   * For CPU backends, this returns the actual pixel buffer.
   * For GPU backends (OpenGL/Vulkan), this may trigger a download or return nullptr.
   */
  virtual uint8_t* data() = 0;
  virtual const uint8_t* data() const = 0;

  /**
   * @brief Get pixel data size in bytes.
   */
  virtual size_t sizeBytes() const = 0;

  /**
   * @brief Copy pixel data from source buffer to framebuffer.
   * @param sourceData RGBA pixel data (width * height * 4 bytes)
   * @param numBytes Size of source data in bytes
   *
   * For GPU backends, this may upload data to GPU.
   */
  virtual void upload(const uint8_t* sourceData, size_t numBytes) = 0;

  /**
   * @brief Copy framebuffer pixel data to destination buffer.
   * @param destData Destination buffer (must be at least sizeBytes())
   * @param numBytes Size of destination buffer in bytes
   *
   * For GPU backends, this may download data from GPU.
   */
  virtual void download(uint8_t* destData, size_t numBytes) const = 0;

  /**
   * @brief Present/display the framebuffer (for windowed backends).
   *
   * For OpenGL: swaps buffers
   * For CPU/File: no-op or writes to file
   */
  virtual void present() = 0;

  /**
   * @brief Clear framebuffer to solid color.
   * @param r Red component (0-255)
   * @param g Green component (0-255)
   * @param b Blue component (0-255)
   * @param a Alpha component (0-255)
   */
  virtual void clear(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0, uint8_t a = 255) = 0;

  /**
   * @brief Resize framebuffer.
   * @param newWidth New width in pixels
   * @param newHeight New height in pixels
   *
   * May allocate new resources. Existing pixel data may be lost.
   */
  virtual void resize(int newWidth, int newHeight) = 0;

  /**
   * @brief Check if this backend supports direct pixel access via data().
   * @return true if data() returns valid pointer, false otherwise
   */
  virtual bool supportsDirectAccess() const = 0;

  /**
   * @brief Get backend type name (for debugging/logging).
   * @return String like "OpenGL", "CPU", "Vulkan", "File"
   */
  virtual const char* backendName() const = 0;
};

/**
 * @brief Create CPU framebuffer (in-memory RGBA buffer).
 * @param width Initial width in pixels
 * @param height Initial height in pixels
 * @return Unique pointer to CPU framebuffer
 */
std::unique_ptr<IFramebuffer> createCPUFramebuffer(int width, int height);

/**
 * @brief Create OpenGL framebuffer (wraps existing OpenGL context).
 * @param width Initial width in pixels
 * @param height Initial height in pixels
 * @return Unique pointer to OpenGL framebuffer
 */
std::unique_ptr<IFramebuffer> createOpenGLFramebuffer(int width, int height);

/**
 * @brief Create file framebuffer (writes frames to PNG files).
 * @param width Framebuffer width in pixels
 * @param height Framebuffer height in pixels
 * @param outputPath Directory or filename pattern (e.g., "frames/frame_%05d.png")
 * @return Unique pointer to file framebuffer
 */
std::unique_ptr<IFramebuffer> createFileFramebuffer(int width, int height, const std::string& outputPath);

}  // namespace core
}  // namespace avs
