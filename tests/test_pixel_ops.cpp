#include <array>

#include <gtest/gtest.h>

#include "avs/core.hpp"

namespace {

avs::FrameBufferView makeBuffer(std::array<uint8_t, 3 * 3 * 4>& data) {
  for (int y = 0; y < 3; ++y) {
    for (int x = 0; x < 3; ++x) {
      size_t idx = static_cast<size_t>(y * 3 + x) * 4u;
      data[idx + 0] = static_cast<uint8_t>(x * 40 + y);
      data[idx + 1] = static_cast<uint8_t>(y * 60 + x);
      data[idx + 2] = static_cast<uint8_t>(x * 20 + y * 10);
      data[idx + 3] = static_cast<uint8_t>(200 + x + y);
    }
  }
  avs::FrameBufferView view{};
  view.data = data.data();
  view.width = 3;
  view.height = 3;
  view.stride = 3 * 4;
  return view;
}

}  // namespace

TEST(PixelOps, SampleNearestClamp) {
  std::array<uint8_t, 3 * 3 * 4> data{};
  avs::FrameBufferView view = makeBuffer(data);
  avs::SampleOptions opt;
  opt.filter = avs::Filter::Nearest;
  opt.wrap = avs::Wrap::Clamp;

  avs::ColorRGBA8 c = avs::sampleRGBA(view, -1.0f, -1.0f, opt);
  EXPECT_EQ(data[0], c.r);
  EXPECT_EQ(data[1], c.g);
  EXPECT_EQ(data[2], c.b);
  EXPECT_EQ(data[3], c.a);

  c = avs::sampleRGBA(view, 1.0f, 1.0f, opt);
  size_t center = (1 * 3 + 1) * 4u;
  EXPECT_EQ(data[center + 0], c.r);
  EXPECT_EQ(data[center + 1], c.g);
  EXPECT_EQ(data[center + 2], c.b);
  EXPECT_EQ(data[center + 3], c.a);
}

TEST(PixelOps, SampleBilinearWrap) {
  std::array<uint8_t, 3 * 3 * 4> data{};
  avs::FrameBufferView view = makeBuffer(data);
  avs::SampleOptions opt;
  opt.filter = avs::Filter::Bilinear;
  opt.wrap = avs::Wrap::Wrap;

  avs::ColorRGBA8 c = avs::sampleRGBA(view, 2.5f, 2.5f, opt);
  size_t idx00 = (2 * 3 + 2) * 4u;
  size_t idx10 = (2 * 3 + 0) * 4u;
  EXPECT_NEAR((data[idx00 + 0] + data[idx10 + 0]) / 2.0, c.r, 2.0);
  EXPECT_NEAR((data[idx00 + 1] + data[idx10 + 1]) / 2.0, c.g, 2.0);
}

TEST(PixelOps, BlendModes) {
  avs::ColorRGBA8 dst{100, 120, 140, 200};
  avs::ColorRGBA8 src{50, 60, 70, 128};

  avs::ColorRGBA8 blended = dst;
  avs::blendPixel(blended, src, avs::BlendMode::Add);
  EXPECT_EQ(150, blended.r);
  EXPECT_EQ(180, blended.g);
  EXPECT_EQ(210, blended.b);

  blended = dst;
  avs::blendPixel(blended, src, avs::BlendMode::Multiply);
  EXPECT_LT(blended.r, dst.r);
  EXPECT_LT(blended.g, dst.g);
  EXPECT_LT(blended.b, dst.b);

  blended = dst;
  avs::blendPixel(blended, src, avs::BlendMode::Replace);
  EXPECT_EQ(src.r, blended.r);
  EXPECT_EQ(src.g, blended.g);
  EXPECT_EQ(src.b, blended.b);
}

