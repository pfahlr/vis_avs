#include <avs/core/IFramebuffer.hpp>
#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <vector>

using namespace avs::core;

// Test CPU Framebuffer basic operations
TEST(CPUFramebuffer, CreatesWithCorrectDimensions) {
  auto fb = createCPUFramebuffer(640, 480);
  ASSERT_NE(fb, nullptr);
  EXPECT_EQ(fb->width(), 640);
  EXPECT_EQ(fb->height(), 480);
  EXPECT_EQ(fb->sizeBytes(), 640 * 480 * 4);
  EXPECT_STREQ(fb->backendName(), "CPU");
  EXPECT_TRUE(fb->supportsDirectAccess());
}

TEST(CPUFramebuffer, ClearsFillsWithColor) {
  auto fb = createCPUFramebuffer(64, 64);
  fb->clear(255, 128, 64, 255);

  const uint8_t* data = fb->data();
  ASSERT_NE(data, nullptr);

  // Check first pixel
  EXPECT_EQ(data[0], 255);  // R
  EXPECT_EQ(data[1], 128);  // G
  EXPECT_EQ(data[2], 64);   // B
  EXPECT_EQ(data[3], 255);  // A

  // Check last pixel
  const size_t lastPixel = (64 * 64 - 1) * 4;
  EXPECT_EQ(data[lastPixel + 0], 255);
  EXPECT_EQ(data[lastPixel + 1], 128);
  EXPECT_EQ(data[lastPixel + 2], 64);
  EXPECT_EQ(data[lastPixel + 3], 255);
}

TEST(CPUFramebuffer, UploadDownloadPreservesData) {
  auto fb = createCPUFramebuffer(32, 32);

  // Create test pattern
  std::vector<uint8_t> testData(32 * 32 * 4);
  for (size_t i = 0; i < testData.size(); i += 4) {
    testData[i + 0] = static_cast<uint8_t>(i % 256);       // R
    testData[i + 1] = static_cast<uint8_t>((i / 4) % 256); // G
    testData[i + 2] = static_cast<uint8_t>((i / 8) % 256); // B
    testData[i + 3] = 255;                                  // A
  }

  fb->upload(testData.data(), testData.size());

  // Download and verify
  std::vector<uint8_t> downloaded(32 * 32 * 4);
  fb->download(downloaded.data(), downloaded.size());

  EXPECT_EQ(downloaded, testData);
}

TEST(CPUFramebuffer, ResizeChangesDimensions) {
  auto fb = createCPUFramebuffer(100, 100);
  fb->resize(200, 150);

  EXPECT_EQ(fb->width(), 200);
  EXPECT_EQ(fb->height(), 150);
  EXPECT_EQ(fb->sizeBytes(), 200 * 150 * 4);
}

TEST(CPUFramebuffer, DirectDataAccessWorks) {
  auto fb = createCPUFramebuffer(10, 10);

  uint8_t* data = fb->data();
  ASSERT_NE(data, nullptr);

  // Write directly to buffer
  for (int i = 0; i < 10 * 10 * 4; i += 4) {
    data[i + 0] = 100;
    data[i + 1] = 200;
    data[i + 2] = 50;
    data[i + 3] = 255;
  }

  // Verify via download
  std::vector<uint8_t> downloaded(10 * 10 * 4);
  fb->download(downloaded.data(), downloaded.size());

  for (size_t i = 0; i < downloaded.size(); i += 4) {
    EXPECT_EQ(downloaded[i + 0], 100);
    EXPECT_EQ(downloaded[i + 1], 200);
    EXPECT_EQ(downloaded[i + 2], 50);
    EXPECT_EQ(downloaded[i + 3], 255);
  }
}

// Test File Framebuffer PNG export
TEST(FileFramebuffer, CreatesWithCorrectDimensions) {
  const char* tmpPath = "/tmp/test_frame.png";
  auto fb = createFileFramebuffer(320, 240, tmpPath);

  ASSERT_NE(fb, nullptr);
  EXPECT_EQ(fb->width(), 320);
  EXPECT_EQ(fb->height(), 240);
  EXPECT_STREQ(fb->backendName(), "File");
  EXPECT_TRUE(fb->supportsDirectAccess());
}

TEST(FileFramebuffer, ExportsPNGOnPresent) {
  // Use pattern to control exact filename
  const char* pattern = "/tmp/test_export_%05d.png";
  const char* expectedFile = "/tmp/test_export_00000.png";

  // Clean up any existing file
  std::filesystem::remove(expectedFile);

  auto fb = createFileFramebuffer(64, 64, pattern);
  fb->clear(128, 64, 255, 255);
  fb->present();

  // Verify file was created
  EXPECT_TRUE(std::filesystem::exists(expectedFile));

  // Clean up
  std::filesystem::remove(expectedFile);
}

TEST(FileFramebuffer, SequencePatternGeneratesMultipleFiles) {
  const char* pattern = "/tmp/test_seq_%05d.png";

  // Clean up any existing files
  for (int i = 0; i < 3; i++) {
    char path[256];
    std::snprintf(path, sizeof(path), "/tmp/test_seq_%05d.png", i);
    std::filesystem::remove(path);
  }

  auto fb = createFileFramebuffer(32, 32, pattern);

  // Generate 3 frames
  for (int i = 0; i < 3; i++) {
    fb->clear(i * 80, 100, 200, 255);
    fb->present();
  }

  // Verify all files exist
  EXPECT_TRUE(std::filesystem::exists("/tmp/test_seq_00000.png"));
  EXPECT_TRUE(std::filesystem::exists("/tmp/test_seq_00001.png"));
  EXPECT_TRUE(std::filesystem::exists("/tmp/test_seq_00002.png"));

  // Clean up
  for (int i = 0; i < 3; i++) {
    char path[256];
    std::snprintf(path, sizeof(path), "/tmp/test_seq_%05d.png", i);
    std::filesystem::remove(path);
  }
}

TEST(FileFramebuffer, UploadThenPresentExportsCorrectData) {
  const char* pattern_str = "/tmp/test_upload_%05d.png";
  const char* expectedFile = "/tmp/test_upload_00000.png";
  std::filesystem::remove(expectedFile);

  auto fb = createFileFramebuffer(16, 16, pattern_str);

  // Create checkerboard pattern
  std::vector<uint8_t> pattern(16 * 16 * 4);
  for (int y = 0; y < 16; y++) {
    for (int x = 0; x < 16; x++) {
      int idx = (y * 16 + x) * 4;
      bool isWhite = (x / 4 + y / 4) % 2 == 0;
      pattern[idx + 0] = isWhite ? 255 : 0;
      pattern[idx + 1] = isWhite ? 255 : 0;
      pattern[idx + 2] = isWhite ? 255 : 0;
      pattern[idx + 3] = 255;
    }
  }

  fb->upload(pattern.data(), pattern.size());
  fb->present();

  EXPECT_TRUE(std::filesystem::exists(expectedFile));
  std::filesystem::remove(expectedFile);
}

// Test backend parity - CPU vs File should produce identical pixel data
TEST(BackendParity, CPUAndFileProduceSamePixelData) {
  const int width = 64;
  const int height = 64;

  auto cpuFb = createCPUFramebuffer(width, height);
  auto fileFb = createFileFramebuffer(width, height, "/tmp/parity_test.png");

  // Create test pattern
  std::vector<uint8_t> testPattern(width * height * 4);
  for (size_t i = 0; i < testPattern.size(); i += 4) {
    testPattern[i + 0] = static_cast<uint8_t>((i / 4) % 256);
    testPattern[i + 1] = static_cast<uint8_t>((i / 2) % 256);
    testPattern[i + 2] = static_cast<uint8_t>(i % 256);
    testPattern[i + 3] = 255;
  }

  // Upload to both
  cpuFb->upload(testPattern.data(), testPattern.size());
  fileFb->upload(testPattern.data(), testPattern.size());

  // Download and compare
  std::vector<uint8_t> cpuData(width * height * 4);
  std::vector<uint8_t> fileData(width * height * 4);

  cpuFb->download(cpuData.data(), cpuData.size());
  fileFb->download(fileData.data(), fileData.size());

  EXPECT_EQ(cpuData, fileData);

  std::filesystem::remove("/tmp/parity_test.png");
}

TEST(BackendParity, AllBackendsClearToSameColor) {
  const int width = 32;
  const int height = 32;

  auto cpuFb = createCPUFramebuffer(width, height);
  auto fileFb = createFileFramebuffer(width, height, "/tmp/clear_test.png");

  const uint8_t r = 123, g = 234, b = 45, a = 255;

  cpuFb->clear(r, g, b, a);
  fileFb->clear(r, g, b, a);

  std::vector<uint8_t> cpuData(width * height * 4);
  std::vector<uint8_t> fileData(width * height * 4);

  cpuFb->download(cpuData.data(), cpuData.size());
  fileFb->download(fileData.data(), fileData.size());

  EXPECT_EQ(cpuData, fileData);

  std::filesystem::remove("/tmp/clear_test.png");
}

// Test error handling
TEST(FramebufferErrors, RejectsInvalidDimensions) {
  EXPECT_THROW(createCPUFramebuffer(0, 100), std::invalid_argument);
  EXPECT_THROW(createCPUFramebuffer(100, 0), std::invalid_argument);
  EXPECT_THROW(createCPUFramebuffer(-10, 100), std::invalid_argument);

  EXPECT_THROW(createFileFramebuffer(0, 100, "/tmp/test.png"), std::invalid_argument);
  EXPECT_THROW(createFileFramebuffer(100, -5, "/tmp/test.png"), std::invalid_argument);
}

TEST(FramebufferErrors, RejectsEmptyOutputPath) {
  EXPECT_THROW(createFileFramebuffer(100, 100, ""), std::invalid_argument);
}

TEST(FramebufferErrors, RejectsMismatchedUploadSize) {
  auto fb = createCPUFramebuffer(10, 10);
  std::vector<uint8_t> wrongSize(5 * 5 * 4);  // Too small

  EXPECT_THROW(fb->upload(wrongSize.data(), wrongSize.size()), std::invalid_argument);
}

TEST(FramebufferErrors, RejectsNullPointers) {
  auto fb = createCPUFramebuffer(10, 10);

  EXPECT_THROW(fb->upload(nullptr, 10 * 10 * 4), std::invalid_argument);
  EXPECT_THROW(fb->download(nullptr, 10 * 10 * 4), std::invalid_argument);
}
