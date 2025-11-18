#include <gtest/gtest.h>

#include <avs/core/IFramebuffer.hpp>
#include <avs/core/RenderContext.hpp>
#include <avs/effects/prime/Clear.hpp>

namespace {

// Test that RenderContext can use IFramebuffer backend
TEST(RenderContextFramebuffer, ClearEffectUsesFramebufferBackend) {
  constexpr int kWidth = 64;
  constexpr int kHeight = 48;

  // Create a CPU framebuffer (headless rendering)
  auto framebuffer = avs::core::createCPUFramebuffer(kWidth, kHeight);
  ASSERT_NE(framebuffer, nullptr);
  EXPECT_EQ(framebuffer->width(), kWidth);
  EXPECT_EQ(framebuffer->height(), kHeight);

  // Fill with red (255, 0, 0, 255)
  framebuffer->clear(255, 0, 0, 255);

  // Create render context pointing to the framebuffer
  avs::core::RenderContext context{};
  context.width = kWidth;
  context.height = kHeight;
  context.frameIndex = 0;
  context.deltaSeconds = 1.0 / 60.0;
  context.framebufferBackend = framebuffer.get();

  // Also set legacy view for backward compatibility
  context.framebuffer.data = framebuffer->data();
  context.framebuffer.size = framebuffer->sizeBytes();

  // Verify initial state (red)
  const uint8_t* pixels = framebuffer->data();
  EXPECT_EQ(pixels[0], 255);  // R
  EXPECT_EQ(pixels[1], 0);    // G
  EXPECT_EQ(pixels[2], 0);    // B
  EXPECT_EQ(pixels[3], 255);  // A

  // Create Clear effect and set it to black (0x00000000)
  avs::effects::Clear clearEffect;
  avs::core::ParamBlock params;
  params.setInt("value", 0);
  clearEffect.setParams(params);

  // Render the effect
  ASSERT_TRUE(clearEffect.render(context));

  // Verify framebuffer was cleared to black
  EXPECT_EQ(pixels[0], 0);    // R
  EXPECT_EQ(pixels[1], 0);    // G
  EXPECT_EQ(pixels[2], 0);    // B
  EXPECT_EQ(pixels[3], 0);    // A

  // Verify all pixels are black
  const size_t pixelCount = static_cast<size_t>(kWidth) * kHeight;
  for (size_t i = 0; i < pixelCount * 4; ++i) {
    EXPECT_EQ(pixels[i], 0) << "Pixel byte " << i << " should be 0";
  }
}

// Test backward compatibility - effects using legacy framebuffer view still work
TEST(RenderContextFramebuffer, LegacyFramebufferViewStillWorks) {
  constexpr int kWidth = 32;
  constexpr int kHeight = 24;

  // Create pixel buffer manually (old way)
  std::vector<std::uint8_t> pixels(static_cast<size_t>(kWidth) * kHeight * 4, 128);

  // Create render context with only legacy view (no backend)
  avs::core::RenderContext context{};
  context.width = kWidth;
  context.height = kHeight;
  context.framebuffer.data = pixels.data();
  context.framebuffer.size = pixels.size();
  context.framebufferBackend = nullptr;  // No backend set

  // Clear effect should still work with legacy view
  avs::effects::Clear clearEffect;
  avs::core::ParamBlock params;
  params.setInt("value", 64);
  clearEffect.setParams(params);

  ASSERT_TRUE(clearEffect.render(context));

  // Verify all pixels are set to 64
  for (const auto& pixel : pixels) {
    EXPECT_EQ(pixel, 64);
  }
}

// Test that framebuffer backend metadata is accessible from RenderContext
TEST(RenderContextFramebuffer, BackendMetadataAccessible) {
  auto cpuFramebuffer = avs::core::createCPUFramebuffer(320, 240);
  ASSERT_NE(cpuFramebuffer, nullptr);

  avs::core::RenderContext context{};
  context.framebufferBackend = cpuFramebuffer.get();

  // Effects can query backend properties
  EXPECT_STREQ(context.framebufferBackend->backendName(), "CPU");
  EXPECT_TRUE(context.framebufferBackend->supportsDirectAccess());
  EXPECT_EQ(context.framebufferBackend->width(), 320);
  EXPECT_EQ(context.framebufferBackend->height(), 240);
}

}  // namespace
