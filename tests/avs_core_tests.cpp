#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>

#include "avs/effects.hpp"
#include "avs/fs.hpp"
#include "avs/preset.hpp"

using namespace avs;

TEST(BlurEffect, SpreadsLight) {
  Framebuffer in;
  in.w = 3;
  in.h = 1;
  in.rgba.assign(3 * 4, 0);
  in.rgba[4 + 0] = 255;
  in.rgba[4 + 1] = 255;
  in.rgba[4 + 2] = 255;
  in.rgba[4 + 3] = 255;
  Framebuffer out;
  BlurEffect blur(1);
  blur.init(in.w, in.h);
  blur.process(in, out);
  EXPECT_GT(out.rgba[0], 0);
  EXPECT_LT(out.rgba[0], out.rgba[4]);
}

TEST(ConvolutionEffect, PreservesConstantColor) {
  Framebuffer in;
  in.w = 2;
  in.h = 2;
  in.rgba.assign(2 * 2 * 4, 0);
  for (size_t i = 0; i < in.rgba.size(); i += 4) {
    in.rgba[i + 0] = 100;
    in.rgba[i + 1] = 100;
    in.rgba[i + 2] = 100;
    in.rgba[i + 3] = 255;
  }
  Framebuffer out;
  ConvolutionEffect conv;
  conv.init(in.w, in.h);
  conv.process(in, out);
  for (size_t i = 0; i < out.rgba.size(); i += 4) {
    EXPECT_EQ(out.rgba[i + 0], 100);
    EXPECT_EQ(out.rgba[i + 1], 100);
    EXPECT_EQ(out.rgba[i + 2], 100);
    EXPECT_EQ(out.rgba[i + 3], 255);
  }
}

TEST(ColorMapEffect, ProducesColor) {
  Framebuffer in;
  in.w = 1;
  in.h = 1;
  in.rgba = {128, 128, 128, 255};
  Framebuffer out;
  ColorMapEffect cm;
  cm.init(in.w, in.h);
  cm.process(in, out);
  EXPECT_NE(out.rgba[0], out.rgba[1]);
  EXPECT_EQ(out.rgba[3], 255);
}

static std::uint32_t checksum(const Framebuffer& fb) {
  std::uint32_t sum = 0;
  for (auto v : fb.rgba) {
    sum += v;
  }
  return sum;
}

TEST(MotionBlurEffect, AveragesWithHistory) {
  Framebuffer in;
  in.w = 1;
  in.h = 1;
  in.rgba = {100, 0, 0, 50};
  Framebuffer out;
  MotionBlurEffect mb;
  mb.init(in.w, in.h);
  mb.process(in, out);
  EXPECT_EQ(checksum(out), 75u);
}

TEST(ColorTransformEffect, InvertsColor) {
  Framebuffer in;
  in.w = 1;
  in.h = 1;
  in.rgba = {10, 20, 30, 40};
  Framebuffer out;
  ColorTransformEffect ct;
  ct.init(in.w, in.h);
  ct.process(in, out);
  EXPECT_EQ(checksum(out), 920u);
}

TEST(GlowEffect, Brightens) {
  Framebuffer in;
  in.w = 1;
  in.h = 1;
  in.rgba = {200, 200, 200, 255};
  Framebuffer out;
  GlowEffect g;
  g.process(in, out);
  EXPECT_EQ(checksum(out), 1005u);
}

TEST(ZoomRotateEffect, Rotates180) {
  Framebuffer in;
  in.w = 2;
  in.h = 1;
  in.rgba = {1, 2, 3, 4, 5, 6, 7, 8};
  Framebuffer out;
  ZoomRotateEffect zr;
  zr.init(in.w, in.h);
  zr.process(in, out);
  ASSERT_EQ(out.rgba[0], 5);
  ASSERT_EQ(out.rgba[4], 1);
}

TEST(MirrorEffect, MirrorsHorizontally) {
  Framebuffer in;
  in.w = 2;
  in.h = 1;
  in.rgba = {1, 2, 3, 4, 5, 6, 7, 8};
  Framebuffer out;
  MirrorEffect m;
  m.init(in.w, in.h);
  m.process(in, out);
  ASSERT_EQ(out.rgba[0], 5);
  ASSERT_EQ(out.rgba[4], 1);
}

TEST(TunnelEffect, GeneratesGradient) {
  Framebuffer in;
  in.w = 2;
  in.h = 2;
  in.rgba.assign(2 * 2 * 4, 0);
  Framebuffer out;
  TunnelEffect t;
  t.init(in.w, in.h);
  t.process(in, out);
  EXPECT_EQ(checksum(out), 1038u);
}

TEST(RadialBlurEffect, AveragesWithCenter) {
  Framebuffer in;
  in.w = 2;
  in.h = 2;
  in.rgba = {0, 0, 0, 0, 10, 10, 10, 10, 20, 20, 20, 20, 30, 30, 30, 30};
  Framebuffer out;
  RadialBlurEffect rb;
  rb.init(in.w, in.h);
  rb.process(in, out);
  EXPECT_EQ(checksum(out), 360u);
}

TEST(AdditiveBlendEffect, AddsConstant) {
  Framebuffer in;
  in.w = 1;
  in.h = 1;
  in.rgba = {100, 100, 100, 100};
  Framebuffer out;
  AdditiveBlendEffect ab;
  ab.init(in.w, in.h);
  ab.process(in, out);
  EXPECT_EQ(checksum(out), 440u);
}

TEST(PresetParser, ParsesChainAndReportsUnsupported) {
  auto tmp = std::filesystem::temp_directory_path() / "test.avs";
  {
    std::ofstream(tmp) << "blur radius=2\nunknown\n";
  }
  auto preset = avs::parsePreset(tmp);
  EXPECT_EQ(preset.chain.size(), 2u);
  EXPECT_TRUE(dynamic_cast<avs::BlurEffect*>(preset.chain[0].get()) != nullptr);
  EXPECT_EQ(preset.warnings.size(), 1u);
  std::filesystem::remove(tmp);
}

TEST(FileWatcher, DetectsModification) {
  auto tmp = std::filesystem::temp_directory_path() / "watch.txt";
  {
    std::ofstream(tmp) << "a";
  }
  avs::FileWatcher w(tmp);
  {
    std::ofstream(tmp) << "b";
  }
  bool changed = false;
  for (int i = 0; i < 10 && !changed; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    changed = w.poll();
  }
  EXPECT_TRUE(changed);
  std::filesystem::remove(tmp);
}
