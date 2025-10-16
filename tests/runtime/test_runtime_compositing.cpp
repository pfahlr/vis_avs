#include <gtest/gtest.h>

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#include "avs/core.hpp"
#include "avs/effect.hpp"
#include "avs/effects_misc.hpp"
#include "avs/effects_render.hpp"
#include "avs/effects_trans.hpp"
#include "avs/runtime/framebuffers.h"
#include "runtime/framebuffers.h"

namespace {

void fillFrame(avs::FrameBufferView view, std::uint8_t r, std::uint8_t g, std::uint8_t b) {
  if (!view.data) {
    return;
  }
  for (int y = 0; y < view.height; ++y) {
    std::uint8_t* row = view.data + static_cast<std::size_t>(y) * view.stride;
    for (int x = 0; x < view.width; ++x) {
      std::uint8_t* px = row + x * 4;
      px[0] = r;
      px[1] = g;
      px[2] = b;
      px[3] = 255;
    }
  }
}

std::vector<std::uint8_t> copyFrame(const avs::FrameBufferView& view) {
  std::vector<std::uint8_t> copy;
  if (!view.data || view.width <= 0 || view.height <= 0 || view.stride <= 0) {
    return copy;
  }
  const std::size_t totalBytes = static_cast<std::size_t>(view.height) * view.stride;
  copy.assign(view.data, view.data + totalBytes);
  return copy;
}

}  // namespace

TEST(RuntimeCompositing, SaveRestoreBuffers) {
  avs::runtime::Framebuffers registers(3, 2);
  registers.beginFrame();
  auto views = avs::runtime::makeFrameBuffers(registers);

  ASSERT_NE(views.current.data, nullptr);
  fillFrame(views.current, 10, 20, 30);
  const auto original = copyFrame(views.current);
  ASSERT_FALSE(original.empty());

  avs::SaveBufferEffect save;
  avs::RestoreBufferEffect restore;
  save.set_parameter("slot", std::string("B"));
  restore.set_parameter("slot", std::string("B"));

  avs::TimingInfo timing{};
  avs::AudioFeatures audio{};
  avs::ProcessContext ctx{timing, audio, views, nullptr, nullptr};

  save.process(ctx, views.current);

  std::fill(views.current.data, views.current.data + original.size(), 0u);
  restore.process(ctx, views.current);

  EXPECT_TRUE(std::equal(original.begin(), original.end(), views.current.data));
}

TEST(RuntimeCompositing, PictureEffectMirrorsHorizontally) {
  avs::runtime::Framebuffers registers(2, 2);
  registers.beginFrame();
  auto views = avs::runtime::makeFrameBuffers(registers);

  ASSERT_NE(views.current.data, nullptr);
  std::filesystem::path temp = std::filesystem::temp_directory_path() / "avs_picture_test.png";
  std::array<std::uint8_t, 16> pixels{255, 0, 0, 255, 0, 255, 0, 255,
                                      0, 0, 255, 255, 255, 255, 0, 255};
  ASSERT_TRUE(stbi_write_png(temp.string().c_str(), 2, 2, 4, pixels.data(), 2 * 4));

  avs::PictureEffect picture;
  picture.set_parameter("path", std::string(temp.string()));

  avs::MirrorEffect mirror;
  mirror.set_parameter("mode", std::string("horizontal"));

  avs::TimingInfo timing{};
  avs::AudioFeatures audio{};
  avs::ProcessContext ctx{timing, audio, views, nullptr, nullptr};

  picture.process(ctx, views.current);
  mirror.process(ctx, views.current);

  const std::array<std::uint8_t, 16> expected{0, 255, 0, 255, 255, 0, 0, 255,
                                              255, 255, 0, 255, 0, 0, 255, 255};
  EXPECT_TRUE(std::equal(expected.begin(), expected.end(), views.current.data));

  std::filesystem::remove(temp);
}

TEST(RuntimeCompositing, InterleaveProducesCheckerboard) {
  avs::runtime::Framebuffers registers(4, 4);
  registers.beginFrame();
  auto views = avs::runtime::makeFrameBuffers(registers);

  ASSERT_NE(views.current.data, nullptr);
  avs::InterleaveEffect interleave;
  interleave.set_parameter("frame_count", 2);

  avs::AudioFeatures audio{};
  avs::TimingInfo timing{};

  timing.frame_index = 0;
  avs::ProcessContext ctx0{timing, audio, views, nullptr, nullptr};
  fillFrame(views.current, 10, 20, 30);
  interleave.process(ctx0, views.current);

  registers.beginFrame();
  avs::runtime::refreshFrameBuffers(registers, views);

  timing.frame_index = 1;
  avs::ProcessContext ctx1{timing, audio, views, nullptr, nullptr};
  fillFrame(views.current, 200, 40, 120);
  interleave.process(ctx1, views.current);

  for (int y = 0; y < views.current.height; ++y) {
    for (int x = 0; x < views.current.width; ++x) {
      const std::uint8_t* px = views.current.data + static_cast<std::size_t>(y) * views.current.stride + x * 4;
      const bool expectSecond = ((1 + ((x + y) % 2)) % 2) == 1;
      if (expectSecond) {
        EXPECT_EQ(px[0], 200);
        EXPECT_EQ(px[1], 40);
        EXPECT_EQ(px[2], 120);
      } else {
        EXPECT_EQ(px[0], 10);
        EXPECT_EQ(px[1], 20);
        EXPECT_EQ(px[2], 30);
      }
      EXPECT_EQ(px[3], 255);
    }
  }
}
